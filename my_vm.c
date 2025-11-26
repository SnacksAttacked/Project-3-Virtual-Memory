
#include "my_vm.h"
#include <string.h>   // optional for memcpy if you later implement put/get
#include <pthread.h> //for mutex

// -----------------------------------------------------------------------------
// Global Declarations (optional)
// -----------------------------------------------------------------------------

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mmlock = PTHREAD_MUTEX_INITIALIZER;

struct tlb tlb_store; // Placeholder for your TLB structure

// Optional counters for TLB statistics
static unsigned long long tlb_lookups = 0;
static unsigned long long tlb_misses  = 0;

static char* physical_mem;
static char* virtual_mem;
static char* bit_map;
static char* vbit_map;
static pde_t* directory; 

static volatile int mem_initialized = 0;

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

static void set_bit_at_index(char *bitmap, int index)
{
    int b = index / 8;
    int position = index % 8;
    bitmap[b] = bitmap[b] ^ (1 << position);
    return;
}

static int get_bit_at_index(char *bitmap, int index)
{
    int b = index / 8;
    int shift = index % 8;
    //Get to the location in the character bitmap array
    //Hint: Right shift and AND will be invaluable here!
    return (bitmap[b] >> shift) & 1;
}

/*
 * set_physical_mem()
 * ------------------
 * Allocates and initializes simulated physical memory and any required
 * data structures (e.g., bitmaps for tracking page use).
 *
 * Return value: None.
 * Errors should be handled internally (e.g., failed allocation).
 */
void set_physical_mem() {
    // TODO: Implement memory allocation for simulated physical memory.
    // Use 32-bit values for sizes, page counts, and offsets.
    uint32_t virtual_pages = MAX_MEMSIZE/PGSIZE; 
    uint32_t physical_pages = MEMSIZE/PGSIZE;
    uint32_t BITMAP_SIZE = physical_pages/8;
    uint32_t VBITMAP_SIZE = virtual_pages/8;
    bit_map = (char*)malloc(BITMAP_SIZE);
    memset(bit_map, 0, BITMAP_SIZE);
    vbit_map = (char*)malloc(VBITMAP_SIZE);
    memset(vbit_map, 0, VBITMAP_SIZE);
    physical_mem = (char*)malloc(MEMSIZE);
    memset(physical_mem, 0, MEMSIZE); //zeroes everything out in physical memory. 
    directory = (pde_t*) physical_mem;
    set_bit_at_index(bit_map, 0); //Initializes the page directory
    virtual_mem = (char*)malloc(MAX_MEMSIZE);
    mem_initialized = 1;
}



uint32_t find_free(char* bitmap, uint32_t page_ct, uint32_t pages_needed)
{
    int found = 0;
    uint32_t contiguous = 0;
    uint32_t starting_index = 0;
    uint32_t bit_index = 0;
    while(contiguous < pages_needed && bit_index < page_ct)
    {
        if(get_bit_at_index(bitmap, bit_index) == 0)
        {
            //printf("Bit is free, adding to contiguous!\n");
            if(contiguous == 0)
            {
                starting_index = bit_index;
            }
            contiguous++;
            if(contiguous == pages_needed)
            {
                found = 1;
                return starting_index;
            }
            
        }
        else
        {
            //printf("Bit is not free, resetting!\n");
            contiguous = 0;
        }
        bit_index++;
    }
        //printf("Couldn't find it :/\n");
        return -1;
}

void set_mult_bits(char* bitmap, uint32_t bit_index, uint32_t amount)
{

    while(amount > 0)
    {
        set_bit_at_index(bitmap, bit_index);
        bit_index++;
        amount--;
    }

}

// -----------------------------------------------------------------------------
// TLB
// -----------------------------------------------------------------------------

/*
 * TLB_add()
 * ---------
 * Adds a new virtual-to-physical translation to the TLB.
 * Ensure thread safety when updating shared TLB data.
 *
 * Return:
 *   0  -> Success (translation successfully added)
 *  -1  -> Failure (e.g., TLB full or invalid input)
 */
int TLB_add(void *va, void *pa)
{
    // TODO: Implement TLB insertion logic.

    return -1; // Currently returns failure placeholder.
}

/*
 * TLB_check()
 * -----------
 * Looks up a virtual address in the TLB.
 *
 * Return:
 *   Pointer to the corresponding page table entry (PTE) if found.
 *   NULL if the translation is not found (TLB miss).
 */
pte_t *TLB_check(void *va)
{
    // TODO: Implement TLB lookup.
    return NULL; // Currently returns TLB miss.
}

/*
 * print_TLB_missrate()
 * --------------------
 * Calculates and prints the TLB miss rate.
 *
 * Return value: None.
 */
void print_TLB_missrate(void)
{
    double miss_rate = 0.0;
    // TODO: Calculate miss rate as (tlb_misses / tlb_lookups).
    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}

// -----------------------------------------------------------------------------
// Page Table
// -----------------------------------------------------------------------------

/*
 * translate()
 * -----------
 * Translates a virtual address to a physical address.
 * Perform a TLB lookup first; if not found, walk the page directory
 * and page tables using a two-level lookup.
 *
 * Return:
 *   Pointer to the PTE structure if translation succeeds.
 *   NULL if translation fails (e.g., page not mapped).
 */
pte_t *translate(pde_t *pgdir, void *va)
{
    // TODO: Extract the 32-bit virtual address and compute indices
    // for the page directory, page table, and offset.
    // Return the corresponding PTE if found.
    pte_t* ptr = NULL;
    uint32_t temp = VA2U(va);
    uint32_t dir = PDX(temp);
    uint32_t table = PTX(temp);
    uint32_t offset = OFF(temp);
    
    if (pgdir[dir]){
        long long int num = pgdir[dir];
        pte_t *ptr2 = (pte_t*) ((char*)pgdir+(pgdir[dir] & ~OFFMASK));
        if (ptr2[table]){
            return (pgdir+(ptr2[table] & ~OFFMASK))+offset;
            /*
            num = (num << 10) + ptr[table];
            ptr = &(ptr2[offset]);
            if (ptr){
                num = (num << 12) + offset;
                return ptr;
            }*/
            
        }
    }


    //TODO later: TLB lookup


    return NULL; // Translation unsuccessful placeholder.
}

/*
 * map_page()
 * -----------
 * Establishes a mapping between a virtual and a physical page.
 * Creates intermediate page tables if necessary.
 *
 * Return:
 *   0  -> Success (mapping created)
 *  -1  -> Failure (e.g., no space or invalid address)
 */

//helper function to get avail phys block
void * get_next_phys(){
    uint32_t physical_block = MEMSIZE/PGSIZE;
    uint32_t block_to_start = find_free(bit_map, physical_block, 1);
    if(block_to_start == (uint32_t)-1)
    {
        return NULL; // No available block placeholder.
    }
    set_bit_at_index(bit_map, block_to_start);
    return U2VA(block_to_start*PGSIZE); 
}

int map_page(pde_t *pgdir, void *va, void *pa)
{
    // TODO: Map virtual address to physical address in the page tables.
    uint32_t temp = VA2U(va);
    uint32_t dir = PDX(temp);
    uint32_t table = PTX(temp);
   // uint32_t offset = OFF(temp);

    pde_t *ptr = pgdir;

    if (ptr[dir] == 0){
        uint32_t phys_blk = VA2U(get_next_phys(PGSIZE));
        if (phys_blk == 0){
            return -1; //cound't find a block
        }
        ptr[dir] = phys_blk | 0x1;
    }
    pte_t* page_table = (pte_t*) ((char*)pgdir+(ptr[dir] & ~OFFMASK));

    if (page_table[table] == 0){
         page_table[table] = (pte_t) VA2U(pa);
    }

    if (page_table[table] == NULL){
        return -1; // Failure placeholder.
    }
    
    return 0;
}

// pte_t* offsetptr = &ptr2[table];
                
                // //if page w/ offset is found
                // if (offsetptr[offset] & 1 != 1){
                //     //creates page w/ offset

                //     offsetptr[offset] |= 1;

                //     //links phys and virt (maybe?)
                //     pa = va;
                //     va = offsetptr[offset];
                //     return 0;
                // }

// -----------------------------------------------------------------------------
// Allocation
// -----------------------------------------------------------------------------

/*
 * get_next_avail()
 * ----------------
 * Finds and returns the base virtual address of the next available
 * block of contiguous free pages.
 *
 * Return:
 *   Pointer to the base virtual address if available.
 *   NULL if there are no sufficient free pages.
 */
void *get_next_avail(int num_pages)
{
    uint32_t virtual_pages = MAX_MEMSIZE/PGSIZE;
    uint32_t page_to_start = find_free(vbit_map, virtual_pages, num_pages);
    if(page_to_start == (uint32_t)-1)
    {
        return NULL; // No available block placeholder.
    }
    // TODO: Implement virtual bitmap search for free pages.
    return U2VA(page_to_start*PGSIZE); 
}

/*
 * n_malloc()
 * -----------
 * Allocates a given number of bytes in virtual memory.
 * Initializes physical memory and page directories if not already done.
 *
 * Return:
 *   Pointer to the starting virtual address of allocated memory (success).
 *   NULL if allocation fails.
 */
void *n_malloc(unsigned int num_bytes)
{
    //printf("Size: %zu bytes\n", sizeof(unsigned long long));

    pthread_mutex_lock(&mlock);

    uint32_t virtual_pages = MAX_MEMSIZE/PGSIZE;
    if(mem_initialized == 0)
    {
        set_physical_mem();
    }
    uint32_t pages_needed =  ((num_bytes+PGSIZE-1)/PGSIZE);
    void* base = get_next_avail(pages_needed);
    uint32_t indx = VA2U(base) / PGSIZE;
    set_mult_bits(vbit_map, indx, pages_needed);
    //printf("Translated %p to: %p\n", base, translate(directory, base));
    void* bs = base;
    while (pages_needed > 0)
    {
        void* phys_page = get_next_phys();
        if (map_page(directory, bs, phys_page) == 0){
        }
        else{
        }
        pages_needed--;
        bs = bs+PGSIZE;
    }
    
    pthread_mutex_unlock(&mlock);
    
    // TODO: Determine required pages, allocate them, and map them.
    return base; // Allocation failure placeholder.
}

/*
 * n_free()
 * ---------
 * Frees one or more pages of memory starting at the given virtual address.
 * Marks the corresponding virtual and physical pages as free.
 * Removes the translation from the TLB.
 *
 * Return value: None.
 */
void n_free(void *va, int size)
{
    pthread_mutex_lock(&mlock);

    // TODO: Clear page table entries, update bitmaps, and invalidate TLB.
    int pages_freed =  ((size+PGSIZE-1)/PGSIZE);
    set_mult_bits(vbit_map, (VA2U(va) >> OFFBITS), pages_freed);
    while(pages_freed > 0)
    {
        pte_t* translated_addr = translate(directory, va);
        pde_t* pgdir = directory;
        uint32_t pfn = (translated_addr-pgdir)/PGSIZE;
        if(get_bit_at_index(bit_map, pfn) == 1)
        {
            set_bit_at_index(bit_map, pfn);
        }
        pages_freed--;
        va += PGSIZE;
    }
    
    va = NULL;

    pthread_mutex_unlock(&mlock);

}

// -----------------------------------------------------------------------------
// Data Movement
// -----------------------------------------------------------------------------

/*
 * put_data()
 * ----------
 * Copies data from a user buffer into simulated physical memory using
 * the virtual address. Handle page boundaries properly.
 *
 * Return:
 *   0  -> Success (data written successfully)
 *  -1  -> Failure (e.g., translation failure)
 */
int put_data(void *va, void *val, int size)
{
    pthread_mutex_lock(&mlock);

    void* phys_addr = translate(directory, va);
    if(phys_addr == NULL)
    {
        uint32_t pages_needed = ((size+PGSIZE-1)/PGSIZE);
        while(pages_needed > 0)
        {
            uint32_t start_index = find_free(bit_map, (MEMSIZE/PGSIZE), 1);
            if(start_index == (uint32_t) -1)
            {
                printf("Couldn't find a page\n");
                pthread_mutex_unlock(&mlock);

                return 0;
            }
            set_bit_at_index(bit_map,start_index);
            pages_needed--;
            //map_page(directory, va, phys_addr);
        }
        
    }
    // TODO: Walk virtual pages, translate to physical addresses,
    // and copy data into simulated memory.
    memcpy(phys_addr, val, size);

    pthread_mutex_unlock(&mlock);

    return -1; // Failure placeholder.
}

/*
 * get_data()
 * -----------
 * Copies data from simulated physical memory (accessed via virtual address)
 * into a user buffer.
 *
 * Return value: None.
 */
void get_data(void *va, void *val, int size)
{
    pthread_mutex_lock(&mlock);

    void* phys_addr = translate(directory, va);
    if(phys_addr == NULL)
    {
        pthread_mutex_unlock(&mlock);
        return;
    }
    memcpy(val, phys_addr, size);

    pthread_mutex_unlock(&mlock);

    // TODO: Perform reverse operation of put_data().
    //
}

// -----------------------------------------------------------------------------
// Matrix Multiplication
// -----------------------------------------------------------------------------

/*
 * mat_mult()
 * ----------
 * Performs matrix multiplication of two matrices stored in virtual memory.
 * Each element is accessed and stored using get_data() and put_data().
 *
 * Return value: None.
 */
void mat_mult(void *mat1, void *mat2, int size, void *answer)
{
    int i, j, k;
    uint32_t a, b, c;

    pthread_mutex_lock(&mmlock);

    int (*m1)[size] = (int (*)[size]) mat1;
    int (*m2)[size] = (int (*)[size]) mat2;
    int (*ans)[size] = (int (*)[size]) answer;

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            c = 0;
            for (k = 0; k < size; k++) {
                // TODO: Compute addresses for mat1[i][k] and mat2[k][j].
                // Retrieve values using get_data() and perform multiplication.

                get_data((void*)&m1[i][k], &a, sizeof(int));  // placeholder
                get_data((void*)&m2[k][j], &b, sizeof(int));  // placeholder
                c += (a * b);
            }
            // TODO: Store the result in answer[i][j] using put_data().
            put_data((void*)&ans[i][j], (void *)&c, sizeof(int)); // placeholder
        }
    }

    pthread_mutex_unlock(&mmlock);

}

