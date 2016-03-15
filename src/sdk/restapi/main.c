#include <stdio.h>
#include <curl/curl.h>

int main(int argc, char **argv)
{
	char *gwid = "84eb18afeb0c";
	char *apikey = "YHZbDd0n6TQpPJP2MLBKHM7hn7o=";

	CURL *curl = curl_easy_init();
	if (curl == NULL) {
		printf("curl_easy_init failed\n");
		return -1;
	}

	char *url = "https://api.thingplus.net/gateways/84eb18afeb0c";

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	curl_easy_setopt(curl, CURLOPT_USERNAME, "84eb18afeb0c");
	curl_easy_setopt(curl, CURLOPT_USERPWD, "apikey:YHZbDd0n6TQpPJP2MLBKHM7hn7o=");

	int result = curl_easy_perform(curl);
	printf("%d\n", result);




	return 0;
}
