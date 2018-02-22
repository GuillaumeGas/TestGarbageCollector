#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <cstdint>

using namespace std;

enum Color { WHITE, BLACK };
enum MemType { STACK, HEAP };

struct Range
{
    void * addr;
    size_t size;
    Color color;
};

class Gc
{
public:
    Gc (intptr_t rbp);

    void start();
    void stop();
    void * alloc (size_t size);
    void collect();
    
private:
    void _collect();
    void _freeAll();
    void _scanStack();
    void _scanHeap();
    void _findAndMark(void * ptr, MemType memType);
    void _collectWhites();
    void _printRanges();
    
    bool _enabled;
    std::list<Range*> _ranges;
    intptr_t _rbp;
};

Gc::Gc (intptr_t rbp) : _rbp(rbp), _enabled(false) {}

void Gc::collect() { _collect(); }

void * Gc::alloc (size_t size)
{
    printf ("##### ALLOC #####\n");
    void * mem = malloc (size);

    if (!_enabled)
	return mem;
    
    _ranges.push_back (new Range { mem, size, Color::WHITE });
    printf("pushing back %p\n Starting collect...", mem);
    _collect();
    printf ("ending collect.\n");
    
    return mem;
}

void Gc::start() { _enabled = true; }
void Gc::stop () { _freeAll(); _enabled = false; }

void Gc::_collect()
{
    printf ("[Collect]\n");
    _printRanges();


    _scanStack();
    _scanHeap();

    _collectWhites();
}

void Gc::_scanStack()
{
    // asm ("int3");
    register void * rsp asm ("rsp");
    // void * test = rsp;
    // printf ("rbp : %p\n", _rbp);
    // // printf ("rsptest : %p\n", test);
    // printf ("rsp : %p\n", rsp);

    void * rbp = (void*) _rbp;
    for (void * ptr = rbp; ptr > rsp; ptr -= sizeof (void*)) {
	intptr_t test = *((intptr_t*)ptr);
	void * res = NULL;
	res += test;
	_findAndMark (res, MemType::STACK);
    }
}

void Gc::_scanHeap()
{
    // en fait pour chaque range, on devrait checker sa taille.
    // c'est sans doute pas une bonne idée de lire 4 ou 8 octets
    // à l'adresse pointée par le range.ptr si la mémoire qui avait été
    // allouée à cet endroit n'était que de 2 octets !
    for (auto & it : _ranges) {
	Range * r = it;
	for (void * addr = r->addr; addr < (r->addr + r->size); addr += sizeof (void*)) {
	    void * res = NULL;
	    // on check cette histoire de taille
	    if (it->size == 1) {
		res += (int8_t)*((int8_t*)addr);
	    } else if (it->size == 2) {
		res += (int16_t)*((int16_t*)addr);
	    } else if (it->size == 4) { // allé on va dire qu'on est sur du 64 bits
		res += (int32_t)*((int32_t*)addr);
	    } else {
		res += (int64_t)*((int64_t*)addr);
	    }
	    _findAndMark (res, MemType::HEAP);
	}   
    }
}

void Gc::_findAndMark(void * ptr, MemType memType)
{
    for (auto & it : _ranges) {
	printf ("%p == %p\n", (*it).addr, ptr);
	if ((*it).addr == ptr) {
	    (*it).color = Color::BLACK;
	    printf ("Ptr found ! %p on %s\n", ptr, memType == MemType::STACK ? "stack" : "heap");
	}
    }
}

void Gc::_collectWhites()
{
    for (auto it = _ranges.begin(); it != _ranges.end(); it++) {
	if ((*it)->color == Color::WHITE) {
	    printf ("freeing %p\n", (*it)->addr);
	    free ((*it)->addr);
	    free (*it);
	    _ranges.erase (it);
	    _collectWhites();
	    break;
	} else {
	    (*it)->color = Color::WHITE;
	}
    }
}

void Gc::_freeAll()
{
    std::cout << "Freeing all memory." << std::endl;
    for (auto it = _ranges.begin(); it != _ranges.end(); it++) {
	free ((*it)->addr);
	free (*it);
    }
    _ranges.clear();
}

void Gc::_printRanges()
{
    for (auto & it : _ranges)
	printf ("[%p, %u, %s]\n", (*it).addr, (*it).size, ((*it).color == Color::WHITE ? "WHITE" : "BLACK"));
    printf("\n");
}

/* ### TESTS ### */

struct Test {
    int * a;
};

void test1 (Gc * gc) {
    int * a = (int*) (gc->alloc (sizeof (int)));
}

void test2(Gc * gc) {
    test1(gc);
}

int main()
{
    register void * rbp asm ("rbp");
    Gc gc((intptr_t)rbp);
    gc.start();

    // int * a = (int*) (gc.alloc (sizeof (int)));
    // int b = 2;
    // a = &b;
    // int * c = (int*) (gc.alloc (sizeof (int)));

    // test2(&gc);
    // gc.collect();

    Test * t = (Test*) (gc.alloc (sizeof (Test)));
    t->a = (int*) (gc.alloc (sizeof(int)));
    int d;
    t = (Test*)&d;
    
    cout << "Le test" << endl;
    int * c = (int*) (gc.alloc (sizeof (int)));
    gc.collect();
    
    gc.stop();
}
    
