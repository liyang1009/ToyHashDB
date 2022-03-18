//create by: liyang
//desc :duration the hash data to hard disk
//create time:2020-11-18
#include "stdio.h"
#include <time.h>
#include "stdlib.h"
#include <string.h>
#define TRUE 1
#define BUFFER_SIZE 4096
#define INIT_TABLE_SIZE  50000

typedef struct hash_element{
    char * key;
    struct hash_element * next;
    char * data;
    int size;
    int key_size;
} hash_element;

typedef struct hash_table{
    int size;
    hash_element ** elements;
}hash_table;


typedef struct page_hader{
    int size;
    int alloc_size;
} page_hader;


// in memeory
typedef struct page{
    page_hader heaer;
    char * data ;

} page;


//in the disk
typedef struct block{
    /* data */
} block;


typedef struct hash_index_element
{
    int offsize;
    int code;
    int element_count;
    int data_length;
    hash_element * element;

}hash_index_element;


//hash db in file 
typedef struct hash_db
{
    int size;
    int current;
    hash_index_element ** indexs;
    page  ** pages;
    FILE *fp;
} hash_db;
static void  resize_table(hash_table * table,int size);
void convert_i2s(unsigned char * bytes,uint32_t n);
u_int32_t convert_s2i(unsigned char * bytes);


//convert value to array subscript
unsigned long get_hashcode(char* str) {
    unsigned long i = 0;
    for (int j=0; str[j]; j++)
        i += str[j];
    return i % INIT_TABLE_SIZE;
}


//create hashtable and initlzation it
hash_table * create_hashtable(){

    hash_table * table = malloc(sizeof(hash_table));
    table->size = INIT_TABLE_SIZE+ 1;
    table->elements = malloc(sizeof(hash_element*) * table->size);
    for(int i = 0 ;i < table->size;i++){
        table->elements[i] = NULL;
    }
    return table;
}

hash_element * put_element(hash_table * table,char  * data,int data_size,char * key,int key_size){
    int code = get_hashcode(key);
    if(code > table->size){
       
        if(table->size* 2 > code){
            resize_table(table,table->size * 2);
        }else{
            int new_size = code + 1;
            resize_table(table,new_size);

        }
    }
    hash_element  * element = table->elements [code];

    if(element == NULL){
        element  = malloc(sizeof(hash_element));

        //set data
        char * e_data = malloc(data_size);
        // e_data[data_size] = '\0';
        memcpy(e_data,data,data_size);
        element->data = e_data;
        element->size = data_size;
        //set key
        char * e_key = malloc(key_size);
 
        memcpy(e_key,key,key_size);
        element->key = e_key;
        element->key_size = key_size;
        element->next = NULL;
        table->elements [code] = element;

    }else{

        hash_element * current,*prev;
        current  = element;
        while (current != NULL)
        {
            if(strcmp(key,current->key) == 0){
                //replace data
                //free prev value and the alloc new data
                return current;
            }
            prev = current;
            current = current->next;

        }
        if(current == NULL){
            current  = malloc(sizeof(hash_element));
            current->data = malloc(data_size);
            memcpy(current->data,data,data_size);
            current->size = data_size;

            current->key = malloc(key_size);
            memcpy(current->key,key,key_size);
            current->key_size = key_size;
            current->next = NULL;
            prev->next = current;
            return  current;
        }
 
    }
    return element;
}

//add capacity about the table
static void  resize_table(hash_table * table,int size){
    table->elements =  realloc(table->elements,sizeof(hash_element*) * size);
    for(int i = (table)->size;i < size; i++ ){
        (table)->elements[i] = NULL;
    }
    (table)->size = size;
}

//gereate hash_db from hash_table
static hash_db * config_table(hash_table * table){
    hash_db * db = malloc(sizeof(hash_db));
    db->current = 0;
    db->indexs = malloc(sizeof(hash_index_element) * table->size);
    hash_element * element = NULL;
    for(int index = 0;index < table->size;index ++){
        element = table->elements[index];
        hash_index_element * index_element = malloc(sizeof(hash_index_element));
        index_element->element = element;
        index_element->offsize = db->current;
        if(element == NULL){
            index_element->data_length = 0;
        }else{
            int element_count = 0;
            while(element != NULL){
                //loop whole link list;
                index_element->data_length += element->size;
                index_element->data_length+=  element->key_size;
                //key_size and value size
                index_element->data_length += 8;
                element = element->next;
                element_count++;
                //confuse the data about linklist count and length
            }
            //calculate the  collision account
            index_element->data_length += 4;
            index_element->element_count = element_count;
            

        }
        
        index_element->code = index;
        db->indexs[index] = index_element;
        db->current  += index_element->data_length;

        
    }
    db->size  = table->size;
    return db;
}

//dump the hash_table to disk
void dump_table(hash_table * table,FILE *fp){
    // version（4byte)/time/elementcount/codes & offsize of the element
    hash_db  * db =  config_table(table);

    char  * version = "0.10";
    
    int create_time = (int)time(NULL);
    int head_size = table->size * 12 +12 ;
    db->current = head_size;

    char ori_header_bytes[600028] = {0};
    
    printf("the head size is %d\r\n",head_size);
    char *  header_bytes  = ori_header_bytes;

    memcpy(header_bytes,version,4);
    header_bytes+=4;
    convert_i2s((unsigned char * )(header_bytes),create_time);
    header_bytes+=4;

    convert_i2s((unsigned char * )(header_bytes),table->size);
    header_bytes+=4;
    
    //write header
    hash_index_element * index_element =NULL;
    for (int i = 0; i < db->size; i++){
     
        index_element = db->indexs[i];
        convert_i2s((unsigned char *)header_bytes,i);
        header_bytes+=4;
        convert_i2s((unsigned char *)header_bytes,index_element->offsize + db->current);
        header_bytes+=4;
        convert_i2s((unsigned char *)header_bytes,index_element->data_length);
        

        header_bytes+=4;

    }
    int write_size = fwrite(ori_header_bytes,sizeof(unsigned char),head_size,fp);


    if(write_size  != head_size){
    
       printf("error");
    }
 
    //write body
    hash_element * element,* current = NULL;
    int element_count = 0;
    char element_header[BUFFER_SIZE] = {0};
    char element_value[BUFFER_SIZE] = {0} ;
    char element_count_b[4] = {0};
    int element_header_len,element_value_len;

    for(int index = 0;index < table->size;index++){
        element = table->elements[index];

        current = element;
        if(current == NULL){
            continue;
        }
        element_count = db->indexs[index]->element_count;
        convert_i2s((unsigned char * )element_count_b,element_count);
        fwrite(element_count_b,1,4,fp);
        while (element != NULL)
        {
            char * element_value_b = element_value;
            char * element_header_b = element_header ;

            element_header_len = 0;
            element_value_len = 0;
            /* code */
            current = element;
            //key length
            int key_size = element->key_size;
            convert_i2s((unsigned char * )element_header_b,key_size);
            element_header_b += 4;
            element_header_len += 4;
           
            //data length
            convert_i2s((unsigned char *)element_header_b,element->size);
            element_header_b += 4;
            element_header_len += 4;

            //key
            memcpy(element_value_b,element->key,key_size);
            element_value_len += key_size;
            element_value_b += key_size;

            //data
            memcpy(element_value_b,element->data,element->size);
            element_value_len += element->size;
            element_value_b += element->size;

       
            int write_header_len  = fwrite(element_header,1,element_header_len,fp);
            if(write_header_len == element_header_len){

            }else{
                // need loop write 
            }
         
            int write_value_len = fwrite(element_value,1,element_value_len,fp);
            if(write_value_len == element_value_len){

                if(current != NULL){
                     printf("the key value is %s\r\n",current->key);

                }
    
            }
            element = element->next;

        }

    }

}

//load data from disk
hash_db * load_file(FILE *fp){
    char header[12];
    fread(header,1,12,fp);
    int size = convert_s2i((unsigned char *)(header+8));
    char   * header_data   =  malloc(size * 12 + 12);
    char * header_data_b = header_data;
    int read_header_size = fread(header_data,1,size*12,fp);

    hash_db * db = malloc(sizeof(hash_db));
    db->indexs = malloc(sizeof(hash_index_element*) * size); 
    hash_index_element * index_element  = NULL;
    for(int index = 0;index < size;index++){
        index_element = malloc(sizeof(hash_index_element));
        index_element->code = convert_s2i((unsigned char *)header_data_b);
        header_data_b += 4;
        index_element->offsize = convert_s2i((unsigned  char *)header_data_b );
        header_data_b += 4;
        index_element->data_length = convert_s2i((unsigned char *)header_data_b );
        header_data_b += 4;
        db->indexs[index] = index_element;
        
  
    }

    db->fp = fp;
    db->size = size;
    free(header_data);
    return db;
}


// get bucket from the hash_db
hash_element * get_element(hash_table * table,hash_db * db,char * key){
    
    unsigned char buffer[BUFFER_SIZE * 10];
    unsigned char * forward_buffer = buffer;
    int code  = get_hashcode(key);
    hash_element * element  = NULL;
    hash_element * current = NULL;
    if(code< db->size){
        element =  table->elements[code];
    }


    if(element == NULL && code< db->size){
        hash_index_element * element_index = db->indexs[code];
        
        if(element_index == NULL){
            //error
            return NULL;
        }else{
            //TODO:random read the element data and restore to hash_element
            fseek(db->fp,2,SEEK_SET);
            fseek(db->fp,element_index->offsize,SEEK_SET);

            int read_size = fread(buffer,1,element_index->data_length,db->fp);
            
            int element_count = convert_s2i((unsigned char *)forward_buffer);
            forward_buffer += 4;
            printf("the element count is %d\r\n",element_count);
                //TODO:hash collision
            if(element_index->data_length>0){

                char key_data_buff[BUFFER_SIZE] = {0};
                for(int index = 0;index < element_count;index++){
                    int key_len = convert_s2i((unsigned char *)forward_buffer);
                    forward_buffer += 4;
                    uint32_t value_len =convert_s2i((unsigned char *)forward_buffer);
                    forward_buffer += 4;
                    
                    char * e_key = key_data_buff;
                    // e_key[key_len] = '\0';
                    memcpy(key_data_buff,forward_buffer,key_len);
                    forward_buffer +=key_len;
                    char * data =  key_data_buff+key_len;
                    memcpy(data,forward_buffer,value_len);
                    forward_buffer +=value_len;
            
                    element = put_element(table,data,value_len,e_key,key_len);
                    if(current == NULL){
                        current = element;
                    }

                }
                while (current != NULL)
                {
                    if(strcmp(current->key,key) == 0){
                        break;
                    }
                    current = current->next;
                    /* code */
                }
            }else{
                return NULL;
            }
            
        }

    } 
    return current;
}



void free_db(hash_db * db){

    for (int i = 0; i < db->size; i++)
    {
        /* code */
        hash_index_element  * index_element = db->indexs[i];
        if(index_element != NULL){
            free(index_element);
        }
    }
    free(db);
    
}

void free_table(hash_table * table){
    for (int i = 0; i < table->size; i++)
    {
        /* code */
        hash_element * element = table->elements[i],*current;

        while (element != NULL )
        {

            current = element;
            /* code */
            element = element->next;
            // free(current->data); 
            free(current);
        }
        
    }
    
}

//
void convert_i2s(unsigned char * bytes,uint32_t n){

    bytes[0] = (n >> 24) & 0xFF;
    bytes[1] =  (n >> 16) & 0xFF ;
    bytes[2] = (n >> 8) & 0xFF;
    bytes[3] = n & 0xFF;
}

uint32_t convert_s2i(unsigned char * bytes){
    return  (((uint32_t)bytes[0] << 24) + ((uint32_t)bytes[1]   << 16)  +  ((uint32_t)bytes[2] << 8)  +(uint32_t) bytes[3] ) ;
    
}


void write_vast_data_from_file(hash_table * table,char * file_path){
        FILE * fp = fopen(file_path,"r");


        if(fp){

        }

        fclose(fp);
}

int main(int argc,char ** argv){



    hash_table * table =  create_hashtable();
    int len = strlen("liyinong");
    put_element(table,"liyinong",len,"liyang123",strlen("liyang123"));
    

    len = strlen("aaa");
    put_element(table,"aaa",len,"bbbbbb",strlen("bbbbbb"));


    len = strlen("ddd");
    put_element(table,"ddd",len,"liyang",strlen("liyang"));


    len = strlen("ccc");
    put_element(table,"ccc",len,"liyang1231",strlen("liyang1231"));
    

    len = strlen("eee");
    put_element(table,"eee",len,"bbbbbb1",strlen("bbbbbb1"));


    len = strlen("666666");
    put_element(table,"666666",len,"6666667",strlen("6666667"));



    len = strlen("6666668");
    put_element(table,"6666668",len,"6666660",strlen("6666660"));

    len = strlen("aaaaaaaaaaaaa111111aaaaaaaaaaaaa111111");
    put_element(table,"aaaaaaaaaaaaa111111aaaaaaaaaaaaa111111",len,"6666661",strlen("6666661"));
 


     for (int i = 0; i < 10000; ++i){
        char data[30] = {0}, key[15] = {0};
        sprintf(data, "abcdefg_%d", i);
        sprintf(key, "1234567_%d", i);
        int data_len = strlen(data);
        int key_len = strlen(key);
        put_element(table,data,data_len,key,key_len);

    }
 
    FILE * db_fp = fopen("hash.db","w+");
    dump_table(table,db_fp);
    fclose(db_fp);

    FILE * db_fp_r = fopen("hash.db","r");
    hash_db * db = load_file(db_fp_r);
  
   

    hash_table * load_table = create_hashtable();

    char * key = NULL;
    if(argc>1){
        key = argv[1];
        printf("the key is %s\r\n",key);
        hash_element * element =   get_element(load_table,db,key);
        if(element != NULL){
            printf("the selected key value is %s\r\n",element->data);

        }
    }

    printf(" the hash_table size is %d\r\n",load_table->size);
    fclose(db_fp_r);

    return 0;
}
