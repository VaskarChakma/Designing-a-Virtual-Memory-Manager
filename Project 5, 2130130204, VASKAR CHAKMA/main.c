#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define PAGE_ENTRY 256 //page table entry 2^8
#define PAGE 256// Page size = 2^8
#define TLB_ENTRY 16 //16 TLB entries
#define P_FRAME 128 // Physical frames = 128

typedef union
{
    unsigned short int Add;  //Save 2-byte Address
    struct  // use union and make offset and page 1 byte each
    {
        unsigned char offset;    // offset = 8bit
        unsigned char page;  // page = 8bit
    };
    
}address;

typedef struct list //stack format
{
    unsigned char page;  // page = 8bit
    unsigned char frame; // frame = 8bit
    struct list *next; // Points to the next one
}list;

typedef struct TLB_entry //stack format
{
    int lastUseTime;    // To update TLB in LRU format
    unsigned char page;  // page = 8bit
    unsigned char frame; // frame = 8bit
    bool valid;
}TLB_entry;

void set_TLB_ENTRY(TLB_entry *tlb_entry, int time, unsigned char page, unsigned char frame)
{
    tlb_entry->lastUseTime = time;  
    tlb_entry->page = page; 
    tlb_entry->frame = frame;   
    tlb_entry->valid = true;    
}

TLB_entry TLB_TABLE[TLB_ENTRY]; 

int find_TLB_TABLE(unsigned char page, int num) 
{
    int i;
    for(i = 0; i < TLB_ENTRY; i++)
    {
        if(!TLB_TABLE[i].valid) break;
        if(TLB_TABLE[i].page == page)   
        {
            TLB_TABLE[i].lastUseTime = num; 
            return i;                       
        }
    }
    return -1;  
}

int find_first_TLB()    //Find the least recently used page in TLB.
{
    int i = 0, ret = 0;
    int firstTime = TLB_TABLE[i].lastUseTime;
    for(i = 1; i < TLB_ENTRY; i++)
    {
        if(firstTime > TLB_TABLE[i].lastUseTime)
        {
            firstTime = TLB_TABLE[i].lastUseTime;   
            ret = i;    
        }
    }
    return ret;
}


int TLB_HIT = 0, PAGE_Fault = 0, STACK_SIZE = 0;    
char PAGE_TABLE[PAGE_ENTRY];    
list *LRU_FRAME, *LRU_BEGIN, *LRU_END;  

void push(unsigned char page, unsigned char frame)  
{
    list *NEWEND = (list*)malloc(sizeof(list)); 
    PAGE_TABLE[page] = frame;   
    LRU_END->page = page;   
    LRU_END->frame = frame; 
    LRU_END->next = NEWEND; 
    LRU_END = LRU_END->next;    
}

char pop()
{
    list *temp = LRU_BEGIN->next;   
    unsigned char cframe = PAGE_TABLE[temp->page];
    PAGE_TABLE[temp->page] = -1;    
    
    LRU_BEGIN->next = temp->next;   
    free(temp); 
    return cframe;  
}

char find_and_push(unsigned char page)  
{
    unsigned char cframe;   
    list *prev = LRU_BEGIN;     
    list *temp = LRU_BEGIN->next;   
    while(temp != LRU_END)  
    {
        if(temp->page == page) 
        {
            cframe = temp->frame;   
            prev->next = temp->next;    
            free(temp); 
            break;
        }
        prev = prev->next;
        temp = temp->next;
    }                          
    push(page, cframe);
    return cframe;
}

int main(int argc, char *argv[])
{
    freopen("Physical.txt","w+",stdout);
    unsigned int i, frame = 0;
    unsigned int num=0;
    memset(PAGE_TABLE, -1, sizeof(PAGE_TABLE)); 
    LRU_BEGIN = (list*)malloc(sizeof(list));
    LRU_END = (list*)malloc(sizeof(list));
    LRU_BEGIN->next = LRU_END;
    
    for(i = 0; i < TLB_ENTRY; i++)              
    {
        set_TLB_ENTRY(&TLB_TABLE[i], i - TLB_ENTRY, -1, -1);
        TLB_TABLE[i].valid = false;
    }
    
    FILE *fin;
    fin = fopen(argv[1], "r");  
    
    address vPage, pPage;   // Virtual page, Physical Page
    int TLB_TABLE_IDX;  
    
    while(fscanf(fin, "%d", &vPage.Add) != EOF) 
    {
        num++;  
        
        pPage.offset = vPage.offset;    
        if((TLB_TABLE_IDX = find_TLB_TABLE(vPage.page, num)) != -1)   
        {
            TLB_HIT++; 
            pPage.page = TLB_TABLE[TLB_TABLE_IDX].frame;    
            find_and_push(vPage.page);  
        }
        
        else    
        {

            int idx = find_first_TLB();
            
            if(PAGE_TABLE[vPage.page] == -1) 
            {
                PAGE_Fault++;   
                if(STACK_SIZE < P_FRAME)    
                    STACK_SIZE++;   
                
                else   
                    frame = pop();  
                
                push(vPage.page, frame++);  
            }
            
            else
                find_and_push(vPage.page);  

            pPage.page = PAGE_TABLE[vPage.page];    
            set_TLB_ENTRY(&TLB_TABLE[idx], num, vPage.page, pPage.page);    
        }
        
        printf("Virtual address : %d Physical address : %d\n", vPage.Add, pPage.Add);  
    }
    
    printf("TLB hit ratio : hit (%d) out of (%d) hit total\n",num,TLB_HIT);
    printf("LRU hit ratio : hit (%d) out of (%d) hit total\n",num,PAGE_Fault);
}