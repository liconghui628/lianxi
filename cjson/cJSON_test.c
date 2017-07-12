#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cJSON.h"

int create_js(void)
{
	cJSON *root, *js_body, *js_list;
    root = cJSON_CreateObject();
	cJSON_AddItemToObject(root,"body", js_body = cJSON_CreateArray());
    cJSON_AddItemToArray(js_body, js_list = cJSON_CreateObject());
    cJSON_AddStringToObject(js_list,"name","xiaohui");
    cJSON_AddNumberToObject(js_list,"status",100);
    {
         //        char *s = cJSON_Print(root);
        char *s = cJSON_PrintUnformatted(root);
        if(s){
            printf(" %s \n",s);
            free(s);
        }
    }
    if(root)
        cJSON_Delete(root);

    return 0;
EXIT:
    return -1;
}

int main(int argc, char **argv)
{
   create_js();
   return 0;
}
