#include <stdio.h>
//#include <tidy/tidy.h>
//#include <tidy/tidybuffio.h>
#include <curl/curl.h>
#include<string.h>
#define TITLE_START "<title"
#define TITLE_END "</title>" 
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
	}else strcpy(title,"\0");
  }

  return r;
}

void get_info_about_url(const char*url,char * buf)
//int main(int argc, char **argv)
{

    CURL *curl;
    char curl_errbuf[CURL_ERROR_SIZE];
    int ret;
    char title[1024];
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
    //curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);    // we don't need body
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);


    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &title);

    char *ct;

    ret = curl_easy_perform(curl);
    if(ret == CURLE_OK) {
       char *ct;
       /* ask for the content-type */
      ret = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
      if((CURLE_OK == ret) && ct){
		if( strstr(ct, "text/html") == 0){
			double dl;
    			curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &dl);
			dl/=1024;
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
      sprintf(buf, "%s",curl_errbuf);

    }
    /* clean-up */
    curl_easy_cleanup(curl);



    return ret;
 

  

 
  return 0;
}
#undef TITLE_START
#undef TITLE_END  
