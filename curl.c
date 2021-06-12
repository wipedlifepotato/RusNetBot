#include <stdio.h>
//#include <tidy/tidy.h>
//#include <tidy/tidybuffio.h>
#include <curl/curl.h>
#include<string.h>
//#include<regex.h>
#define TITLE_START "<title"
#define TITLE_END "</title>" 
#define TITLE_SIZE 2056
/* curl write callback, to fill tidy's input buffer...  */
uint write_cb(char *in, uint size, uint nmemb, char * title)
{

  uint r;
  r = size * nmemb;
  char * pTitle;
 //l l printf("in: %s\n", in);
  if( (pTitle=strstr(in, TITLE_START)) != 0){
	//puts("TITLE found");
	//pTitle+=sizeof(TITLE_START);
	pTitle = strstr(pTitle, ">");
	pTitle+=2;
	//printf("%s\n" ,pTitle);
	char *endTitle = strstr(in, TITLE_END);
	if(endTitle != NULL){
		memcpy(title, (pTitle-1), (endTitle-pTitle)+1);
		size_t tSize = strlen(title);
		if(tSize < TITLE_SIZE) 
			title[tSize] = '\0';
		return r;
	}else strcpy(title,"\0");
  }

  return r;
}
#define ELIF else if
#define MAX_ZONE_SIZE 63
#define PROXY_I2P  "http://127.0.0.1:4444"
#define PROXY_ONION "http://127.0.0.1:8118"
/*"socks4a://127.0.0.1:9050"*/


#define GETFILESIZE \
			char *ct;\
     			curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);\
			double dl;\
    			curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &dl);\
			dl/=1024;\

void get_info_about_url(const char*url,char * buf)
//int main(int argc, char **argv)
{
    printf("url: %s\n",url);
    char *t=strstr(url,"."), *zone=t;

    _Bool isPathToFile = (strstr(t, "/") != NULL) ? 1 : 0;
    while( (t = strstr(t,".")) != NULL){
	    	if( !isPathToFile || strstr(t, "/") != NULL ){
			zone = t;
			t++;
		}else break;
    }
    zone++;

    char real_zone[MAX_ZONE_SIZE];
    bzero(real_zone, sizeof(real_zone));
    for(unsigned short z = 0; z < MAX_ZONE_SIZE && zone[z]; z++){
	if(  (zone[z] < 48 || zone[z] > 57) && (zone[z] < 'a' || zone[z] > 'z') ) break;
	real_zone[z]=zone[z]; 
    }
    printf(".%s\n", real_zone);
    puts("CURL");
    CURL *curl;
    char curl_errbuf[CURL_ERROR_SIZE];
    int ret;
    char title[TITLE_SIZE];
    bzero(title,sizeof(title));
    curl = curl_easy_init();
    printf("URL: %s",url);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.96 Safari/537.36");

    //curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);


    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &title);
    
    if(strncmp(real_zone,"i2p", sizeof("i2p")) == 0){
		puts("Is I2P link");
  		curl_easy_setopt(curl, CURLOPT_PROXY, PROXY_I2P);

    }ELIF (strncmp(real_zone,"onion",sizeof("onion")) == 0){
		puts("is onion link");
  		curl_easy_setopt(curl, CURLOPT_PROXY, PROXY_ONION);
    }
    //char *ct;
    ret = curl_easy_perform(curl);
    if(ret == CURLE_OK) {

	GETFILESIZE;
       /* ask for the content-type */
      if((CURLE_OK == ret) && ct){
		if( strstr(ct, "text/html") == 0){
			//GETFILESIZE;
			sprintf(buf, "File, Type: %s @ %.2f Kb", ct, dl);
		}else{
			if(title[0] == 0)
				sprintf(buf, "%s", ct);
			else
				sprintf(buf, "Title: %s", title);
		}
      }else{
		if(title[0] !=0){
			sprintf(buf, "Only title: %s", title);
		}
	}
    }
    else{
      fprintf(stderr, "%s", curl_errbuf);
      if( title[0] == 0 ){
	GETFILESIZE;
	sprintf(buf, "File, Type: %s @ %.2f Kb", ct, dl);
	//sprintf(buf, "%s",curl_errbuf);
      }
      else sprintf(buf, "Title: %s\n", title);

    }
    /* clean-up */
    curl_easy_cleanup(curl);



    return;
}
#undef TITLE_START
#undef TITLE_END  
