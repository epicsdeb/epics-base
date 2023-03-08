#ifndef SB_H
#define SB_H

#include <sstream>

// in-line string builder (eg. for exception messages)
//   throw std::runtime_error(SB()<<"Answer: !"<<42);
struct SB {
    std::ostringstream strm;
    SB() {}
    operator std::string() const { return strm.str(); }
    template<typename T>
    SB& operator<<(T i) { strm<<i; return *this; }
};

#endif // SB_H
