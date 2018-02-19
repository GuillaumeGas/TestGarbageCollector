#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <list>

enum Color { WHITE, BLACK };

struct Range
{
    intptr_t addr;
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
    
private:
    void _collect();
    void _freeAll();
    void _findAndMark(intptr_t ptr);
    void _collectWhites();
    void _printRanges();
    
    bool _enabled;
    std::list<Range*> _ranges;
    intptr_t _rbp;
};

Gc::Gc (intptr_t rbp) : _rbp(rbp), _enabled(false) {}

void * Gc::alloc (size_t size)
{
    printf ("##### ALLOC #####\n");
    void * mem = malloc (size);

    if (!_enabled)
	return mem;
    
    _ranges.push_back (new Range { (intptr_t)mem, size, Color::WHITE });
    printf("pushing back %p\n Starting collect...", mem);
    _collect();
    printf ("ending collect.\n");
    
    return mem;
}

void Gc::start() { _enabled = true; }
void Gc::stop () { _freeAll(); _enabled = false; }

void Gc::_collect()
{
    // asm ("int3");
    register void * rsp asm ("rsp");
    // void * test = rsp;

    printf ("[Collect]\n");
    printf ("rbp : %p\n", _rbp);
    // printf ("rsptest : %p\n", test);
    printf ("rsp : %p\n", rsp);

    _printRanges();

    void * rbp = (void*) _rbp;
    for (void * ptr = rbp; ptr > rsp; ptr -= sizeof (void*))
	_findAndMark ((intptr_t)(*((intptr_t*)ptr)));

    _collectWhites();
}

void Gc::_findAndMark(intptr_t ptr)
{
    for (auto & it : _ranges) {
	printf ("%p == %p\n", (*it).addr, ptr);
	if ((*it).addr == ptr) {
	    (*it).color = Color::BLACK;
	    printf ("Ptr found ! %p\n", ptr);
	}
    }
}

void Gc::_collectWhites()
{
    for (auto it = _ranges.begin(); it != _ranges.end(); it++) {
	if ((*it)->color == Color::WHITE) {
	    printf ("freeing %p\n", (*it)->addr);
	    free ((void*)((*it)->addr));
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
	free ((void*)((*it)->addr));
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

int main()
{
    register void * rbp asm ("rbp");
    Gc gc((intptr_t)rbp);
    gc.start();

    int * a = (int*) (gc.alloc (sizeof (int)));
    int b = 2;
    a = &b;
    int * c = (int*) (gc.alloc (sizeof (int)));
    
    gc.stop();
}
    
