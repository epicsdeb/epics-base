#ifndef HELPER_H
#define HELPER_H

#include <memory>

#define FOREACH(ITERTYPE, IT,END,C) for(ITERTYPE IT=(C).begin(), END=(C).end(); IT!=END; ++IT)

namespace p2p {
#if __cplusplus>=201103L
template<typename T>
using auto_ptr = std::unique_ptr<T>;
#define PTRMOVE(AUTO) std::move(AUTO)
#else
using std::auto_ptr;
#define PTRMOVE(AUTO) (AUTO)
#endif
}

#endif // HELPER_H
