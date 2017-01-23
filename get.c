/**
 * https://dev.twitter.com/rest/reference/get/statuses/user_timeline
 * http://pixelrobotics.com/2013/01/consuming-the-twitter-public-stream-with-libcurl/
 * https://curl.haxx.se/libcurl/c/getinmemory.html
 * http://stackoverflow.com/questions/21023605/initialize-array-of-strings
 * Compile with:
 *
 * gcc -o get get.c -loauth -lcurl
 **/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <oauth.h>
#include <curl/curl.h>

char **TWEETS;

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
  TWEETS = (char**)malloc(10*sizeof(char*));
  int i = 0;
  //allocate space for each tweet(string)
  // here allocate 141 bytes
  for(i = 0; i < 10; i++){
    printf("Initializing 141 bytes for tweet: %d\n", i);
    TWEETS[i] = (char*)malloc(141*sizeof(char));
  }
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

      printf("%lu bytes retrieved\n", (long)chunk.size);
    }

    curl_easy_cleanup(curl);
    free(chunk.memory);
    curl_global_cleanup();

    return 0;
}
