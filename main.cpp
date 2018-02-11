
#include <iostream>
#include <string>

#include "SoA.h"

enum Person {
    kId,
    kName,
    kAge
};

int main() {
    Mallocator mallocator;
    SoA<int64_t, const char*, int> people(&mallocator);

    people.add(0, "Gandalf", 2019); 
    people.add(1, "Frodo", 33);
    people.add(2, "Samwise", 61);

    for (size_t n : people) {
        std::cout << people.get<kName>(n) << "\n";
    }

    return 0;
}
