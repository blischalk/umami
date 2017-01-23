/**
 * https://dev.twitter.com/rest/reference/get/statuses/user_timeline
 * http://pixelrobotics.com/2013/01/consuming-the-twitter-public-stream-with-libcurl/
 *
 * Compile with:
 *
 * gcc -o get -loauth -lcurl get.c
 **/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <oauth.h>
#include <curl/curl.h>

int main(int argc, const char *argv[])
{
    FILE *out;
    if(argc == 2)
    {
        out = fopen(argv[1], "w");
    }
    else if(argc == 1)
    {
        out = stdout;
    }
    else
    {
        printf("usage: %s [outfile]\n", argv[0]);
        return 1;
    }

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

    // In this case, we're not specifying a callback function for
    // handling received data, so libcURL will use the default, which
    // is to write to the file specified in WRITEDATA
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)out);

    // Execute the request!
    int curlstatus = curl_easy_perform(curl);
    printf("curl_easy_perform terminated with status code %d\n", curlstatus);

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    fclose(out);

    return 0;
}
