#define SOUFFLE_GENERATOR_VERSION "2.5"
#include "souffle/CompiledSouffle.h"
#include "souffle/SignalHandler.h"
#include "souffle/SouffleInterface.h"
#include "souffle/datastructure/BTree.h"
#include "souffle/io/IOSystem.h"
#include "souffle/utility/MiscUtil.h"
#include <any>
namespace functors {
extern "C" {
}
} //namespace functors
namespace souffle::t_btree_000_ii__0_1__11__10 {
using namespace souffle;
struct Type {
static constexpr Relation::arity_type Arity = 2;
using t_tuple = Tuple<RamDomain, 2>;
struct t_comparator_0{
 int operator()(const t_tuple& a, const t_tuple& b) const {
  return (ramBitCast<RamSigned>(a[0]) < ramBitCast<RamSigned>(b[0])) ? -1 : (ramBitCast<RamSigned>(a[0]) > ramBitCast<RamSigned>(b[0])) ? 1 :((ramBitCast<RamSigned>(a[1]) < ramBitCast<RamSigned>(b[1])) ? -1 : (ramBitCast<RamSigned>(a[1]) > ramBitCast<RamSigned>(b[1])) ? 1 :(0));
 }
bool less(const t_tuple& a, const t_tuple& b) const {
  return (ramBitCast<RamSigned>(a[0]) < ramBitCast<RamSigned>(b[0]))|| ((ramBitCast<RamSigned>(a[0]) == ramBitCast<RamSigned>(b[0])) && ((ramBitCast<RamSigned>(a[1]) < ramBitCast<RamSigned>(b[1]))));
 }
bool equal(const t_tuple& a, const t_tuple& b) const {
return (ramBitCast<RamSigned>(a[0]) == ramBitCast<RamSigned>(b[0]))&&(ramBitCast<RamSigned>(a[1]) == ramBitCast<RamSigned>(b[1]));
 }
};
using t_ind_0 = btree_set<t_tuple,t_comparator_0>;
t_ind_0 ind_0;
using iterator = t_ind_0::iterator;
struct context {
t_ind_0::operation_hints hints_0_lower;
t_ind_0::operation_hints hints_0_upper;
};
context createContext() { return context(); }
bool insert(const t_tuple& t);
bool insert(const t_tuple& t, context& h);
bool insert(const RamDomain* ramDomain);
bool insert(RamDomain a0,RamDomain a1);
bool contains(const t_tuple& t, context& h) const;
bool contains(const t_tuple& t) const;
std::size_t size() const;
iterator find(const t_tuple& t, context& h) const;
iterator find(const t_tuple& t) const;
range<iterator> lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */, context& /* h */) const;
range<iterator> lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */) const;
range<t_ind_0::iterator> lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper, context& h) const;
range<t_ind_0::iterator> lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper) const;
range<t_ind_0::iterator> lowerUpperRange_10(const t_tuple& lower, const t_tuple& upper, context& h) const;
range<t_ind_0::iterator> lowerUpperRange_10(const t_tuple& lower, const t_tuple& upper) const;
bool empty() const;
std::vector<range<iterator>> partition() const;
void purge();
iterator begin() const;
iterator end() const;
void printStatistics(std::ostream& o) const;
};
} // namespace souffle::t_btree_000_ii__0_1__11__10 
namespace souffle::t_btree_000_ii__0_1__11__10 {
using namespace souffle;
using t_ind_0 = Type::t_ind_0;
using iterator = Type::iterator;
using context = Type::context;
bool Type::insert(const t_tuple& t) {
context h;
return insert(t, h);
}
bool Type::insert(const t_tuple& t, context& h) {
if (ind_0.insert(t, h.hints_0_lower)) {
return true;
} else return false;
}
bool Type::insert(const RamDomain* ramDomain) {
RamDomain data[2];
std::copy(ramDomain, ramDomain + 2, data);
const t_tuple& tuple = reinterpret_cast<const t_tuple&>(data);
context h;
return insert(tuple, h);
}
bool Type::insert(RamDomain a0,RamDomain a1) {
RamDomain data[2] = {a0,a1};
return insert(data);
}
bool Type::contains(const t_tuple& t, context& h) const {
return ind_0.contains(t, h.hints_0_lower);
}
bool Type::contains(const t_tuple& t) const {
context h;
return contains(t, h);
}
std::size_t Type::size() const {
return ind_0.size();
}
iterator Type::find(const t_tuple& t, context& h) const {
return ind_0.find(t, h.hints_0_lower);
}
iterator Type::find(const t_tuple& t) const {
context h;
return find(t, h);
}
range<iterator> Type::lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */, context& /* h */) const {
return range<iterator>(ind_0.begin(),ind_0.end());
}
range<iterator> Type::lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */) const {
return range<iterator>(ind_0.begin(),ind_0.end());
}
range<t_ind_0::iterator> Type::lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper, context& h) const {
t_comparator_0 comparator;
int cmp = comparator(lower, upper);
if (cmp == 0) {
    auto pos = ind_0.find(lower, h.hints_0_lower);
    auto fin = ind_0.end();
    if (pos != fin) {fin = pos; ++fin;}
    return make_range(pos, fin);
}
if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
}
return make_range(ind_0.lower_bound(lower, h.hints_0_lower), ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator> Type::lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper) const {
context h;
return lowerUpperRange_11(lower,upper,h);
}
range<t_ind_0::iterator> Type::lowerUpperRange_10(const t_tuple& lower, const t_tuple& upper, context& h) const {
t_comparator_0 comparator;
int cmp = comparator(lower, upper);
if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
}
return make_range(ind_0.lower_bound(lower, h.hints_0_lower), ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator> Type::lowerUpperRange_10(const t_tuple& lower, const t_tuple& upper) const {
context h;
return lowerUpperRange_10(lower,upper,h);
}
bool Type::empty() const {
return ind_0.empty();
}
std::vector<range<iterator>> Type::partition() const {
return ind_0.getChunks(400);
}
void Type::purge() {
ind_0.clear();
}
iterator Type::begin() const {
return ind_0.begin();
}
iterator Type::end() const {
return ind_0.end();
}
void Type::printStatistics(std::ostream& o) const {
o << " arity 2 direct b-tree index 0 lex-order [0,1]\n";
ind_0.printStats(o);
}
} // namespace souffle::t_btree_000_ii__0_1__11__10 
namespace souffle::t_btree_000_i__0__1 {
using namespace souffle;
struct Type {
static constexpr Relation::arity_type Arity = 1;
using t_tuple = Tuple<RamDomain, 1>;
struct t_comparator_0{
 int operator()(const t_tuple& a, const t_tuple& b) const {
  return (ramBitCast<RamSigned>(a[0]) < ramBitCast<RamSigned>(b[0])) ? -1 : (ramBitCast<RamSigned>(a[0]) > ramBitCast<RamSigned>(b[0])) ? 1 :(0);
 }
bool less(const t_tuple& a, const t_tuple& b) const {
  return (ramBitCast<RamSigned>(a[0]) < ramBitCast<RamSigned>(b[0]));
 }
bool equal(const t_tuple& a, const t_tuple& b) const {
return (ramBitCast<RamSigned>(a[0]) == ramBitCast<RamSigned>(b[0]));
 }
};
using t_ind_0 = btree_set<t_tuple,t_comparator_0>;
t_ind_0 ind_0;
using iterator = t_ind_0::iterator;
struct context {
t_ind_0::operation_hints hints_0_lower;
t_ind_0::operation_hints hints_0_upper;
};
context createContext() { return context(); }
bool insert(const t_tuple& t);
bool insert(const t_tuple& t, context& h);
bool insert(const RamDomain* ramDomain);
bool insert(RamDomain a0);
bool contains(const t_tuple& t, context& h) const;
bool contains(const t_tuple& t) const;
std::size_t size() const;
iterator find(const t_tuple& t, context& h) const;
iterator find(const t_tuple& t) const;
range<iterator> lowerUpperRange_0(const t_tuple& /* lower */, const t_tuple& /* upper */, context& /* h */) const;
range<iterator> lowerUpperRange_0(const t_tuple& /* lower */, const t_tuple& /* upper */) const;
range<t_ind_0::iterator> lowerUpperRange_1(const t_tuple& lower, const t_tuple& upper, context& h) const;
range<t_ind_0::iterator> lowerUpperRange_1(const t_tuple& lower, const t_tuple& upper) const;
bool empty() const;
std::vector<range<iterator>> partition() const;
void purge();
iterator begin() const;
iterator end() const;
void printStatistics(std::ostream& o) const;
};
} // namespace souffle::t_btree_000_i__0__1 
namespace souffle::t_btree_000_i__0__1 {
using namespace souffle;
using t_ind_0 = Type::t_ind_0;
using iterator = Type::iterator;
using context = Type::context;
bool Type::insert(const t_tuple& t) {
context h;
return insert(t, h);
}
bool Type::insert(const t_tuple& t, context& h) {
if (ind_0.insert(t, h.hints_0_lower)) {
return true;
} else return false;
}
bool Type::insert(const RamDomain* ramDomain) {
RamDomain data[1];
std::copy(ramDomain, ramDomain + 1, data);
const t_tuple& tuple = reinterpret_cast<const t_tuple&>(data);
context h;
return insert(tuple, h);
}
bool Type::insert(RamDomain a0) {
RamDomain data[1] = {a0};
return insert(data);
}
bool Type::contains(const t_tuple& t, context& h) const {
return ind_0.contains(t, h.hints_0_lower);
}
bool Type::contains(const t_tuple& t) const {
context h;
return contains(t, h);
}
std::size_t Type::size() const {
return ind_0.size();
}
iterator Type::find(const t_tuple& t, context& h) const {
return ind_0.find(t, h.hints_0_lower);
}
iterator Type::find(const t_tuple& t) const {
context h;
return find(t, h);
}
range<iterator> Type::lowerUpperRange_0(const t_tuple& /* lower */, const t_tuple& /* upper */, context& /* h */) const {
return range<iterator>(ind_0.begin(),ind_0.end());
}
range<iterator> Type::lowerUpperRange_0(const t_tuple& /* lower */, const t_tuple& /* upper */) const {
return range<iterator>(ind_0.begin(),ind_0.end());
}
range<t_ind_0::iterator> Type::lowerUpperRange_1(const t_tuple& lower, const t_tuple& upper, context& h) const {
t_comparator_0 comparator;
int cmp = comparator(lower, upper);
if (cmp == 0) {
    auto pos = ind_0.find(lower, h.hints_0_lower);
    auto fin = ind_0.end();
    if (pos != fin) {fin = pos; ++fin;}
    return make_range(pos, fin);
}
if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
}
return make_range(ind_0.lower_bound(lower, h.hints_0_lower), ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator> Type::lowerUpperRange_1(const t_tuple& lower, const t_tuple& upper) const {
context h;
return lowerUpperRange_1(lower,upper,h);
}
bool Type::empty() const {
return ind_0.empty();
}
std::vector<range<iterator>> Type::partition() const {
return ind_0.getChunks(400);
}
void Type::purge() {
ind_0.clear();
}
iterator Type::begin() const {
return ind_0.begin();
}
iterator Type::end() const {
return ind_0.end();
}
void Type::printStatistics(std::ostream& o) const {
o << " arity 1 direct b-tree index 0 lex-order [0]\n";
ind_0.printStats(o);
}
} // namespace souffle::t_btree_000_i__0__1 
namespace souffle::t_btree_000_ii__1_0__11__01 {
using namespace souffle;
struct Type {
static constexpr Relation::arity_type Arity = 2;
using t_tuple = Tuple<RamDomain, 2>;
struct t_comparator_0{
 int operator()(const t_tuple& a, const t_tuple& b) const {
  return (ramBitCast<RamSigned>(a[1]) < ramBitCast<RamSigned>(b[1])) ? -1 : (ramBitCast<RamSigned>(a[1]) > ramBitCast<RamSigned>(b[1])) ? 1 :((ramBitCast<RamSigned>(a[0]) < ramBitCast<RamSigned>(b[0])) ? -1 : (ramBitCast<RamSigned>(a[0]) > ramBitCast<RamSigned>(b[0])) ? 1 :(0));
 }
bool less(const t_tuple& a, const t_tuple& b) const {
  return (ramBitCast<RamSigned>(a[1]) < ramBitCast<RamSigned>(b[1]))|| ((ramBitCast<RamSigned>(a[1]) == ramBitCast<RamSigned>(b[1])) && ((ramBitCast<RamSigned>(a[0]) < ramBitCast<RamSigned>(b[0]))));
 }
bool equal(const t_tuple& a, const t_tuple& b) const {
return (ramBitCast<RamSigned>(a[1]) == ramBitCast<RamSigned>(b[1]))&&(ramBitCast<RamSigned>(a[0]) == ramBitCast<RamSigned>(b[0]));
 }
};
using t_ind_0 = btree_set<t_tuple,t_comparator_0>;
t_ind_0 ind_0;
using iterator = t_ind_0::iterator;
struct context {
t_ind_0::operation_hints hints_0_lower;
t_ind_0::operation_hints hints_0_upper;
};
context createContext() { return context(); }
bool insert(const t_tuple& t);
bool insert(const t_tuple& t, context& h);
bool insert(const RamDomain* ramDomain);
bool insert(RamDomain a0,RamDomain a1);
bool contains(const t_tuple& t, context& h) const;
bool contains(const t_tuple& t) const;
std::size_t size() const;
iterator find(const t_tuple& t, context& h) const;
iterator find(const t_tuple& t) const;
range<iterator> lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */, context& /* h */) const;
range<iterator> lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */) const;
range<t_ind_0::iterator> lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper, context& h) const;
range<t_ind_0::iterator> lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper) const;
range<t_ind_0::iterator> lowerUpperRange_01(const t_tuple& lower, const t_tuple& upper, context& h) const;
range<t_ind_0::iterator> lowerUpperRange_01(const t_tuple& lower, const t_tuple& upper) const;
bool empty() const;
std::vector<range<iterator>> partition() const;
void purge();
iterator begin() const;
iterator end() const;
void printStatistics(std::ostream& o) const;
};
} // namespace souffle::t_btree_000_ii__1_0__11__01 
namespace souffle::t_btree_000_ii__1_0__11__01 {
using namespace souffle;
using t_ind_0 = Type::t_ind_0;
using iterator = Type::iterator;
using context = Type::context;
bool Type::insert(const t_tuple& t) {
context h;
return insert(t, h);
}
bool Type::insert(const t_tuple& t, context& h) {
if (ind_0.insert(t, h.hints_0_lower)) {
return true;
} else return false;
}
bool Type::insert(const RamDomain* ramDomain) {
RamDomain data[2];
std::copy(ramDomain, ramDomain + 2, data);
const t_tuple& tuple = reinterpret_cast<const t_tuple&>(data);
context h;
return insert(tuple, h);
}
bool Type::insert(RamDomain a0,RamDomain a1) {
RamDomain data[2] = {a0,a1};
return insert(data);
}
bool Type::contains(const t_tuple& t, context& h) const {
return ind_0.contains(t, h.hints_0_lower);
}
bool Type::contains(const t_tuple& t) const {
context h;
return contains(t, h);
}
std::size_t Type::size() const {
return ind_0.size();
}
iterator Type::find(const t_tuple& t, context& h) const {
return ind_0.find(t, h.hints_0_lower);
}
iterator Type::find(const t_tuple& t) const {
context h;
return find(t, h);
}
range<iterator> Type::lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */, context& /* h */) const {
return range<iterator>(ind_0.begin(),ind_0.end());
}
range<iterator> Type::lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */) const {
return range<iterator>(ind_0.begin(),ind_0.end());
}
range<t_ind_0::iterator> Type::lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper, context& h) const {
t_comparator_0 comparator;
int cmp = comparator(lower, upper);
if (cmp == 0) {
    auto pos = ind_0.find(lower, h.hints_0_lower);
    auto fin = ind_0.end();
    if (pos != fin) {fin = pos; ++fin;}
    return make_range(pos, fin);
}
if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
}
return make_range(ind_0.lower_bound(lower, h.hints_0_lower), ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator> Type::lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper) const {
context h;
return lowerUpperRange_11(lower,upper,h);
}
range<t_ind_0::iterator> Type::lowerUpperRange_01(const t_tuple& lower, const t_tuple& upper, context& h) const {
t_comparator_0 comparator;
int cmp = comparator(lower, upper);
if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
}
return make_range(ind_0.lower_bound(lower, h.hints_0_lower), ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator> Type::lowerUpperRange_01(const t_tuple& lower, const t_tuple& upper) const {
context h;
return lowerUpperRange_01(lower,upper,h);
}
bool Type::empty() const {
return ind_0.empty();
}
std::vector<range<iterator>> Type::partition() const {
return ind_0.getChunks(400);
}
void Type::purge() {
ind_0.clear();
}
iterator Type::begin() const {
return ind_0.begin();
}
iterator Type::end() const {
return ind_0.end();
}
void Type::printStatistics(std::ostream& o) const {
o << " arity 2 direct b-tree index 0 lex-order [1,0]\n";
ind_0.printStats(o);
}
} // namespace souffle::t_btree_000_ii__1_0__11__01 
namespace souffle::t_btree_000_ii__0_1__11 {
using namespace souffle;
struct Type {
static constexpr Relation::arity_type Arity = 2;
using t_tuple = Tuple<RamDomain, 2>;
struct t_comparator_0{
 int operator()(const t_tuple& a, const t_tuple& b) const {
  return (ramBitCast<RamSigned>(a[0]) < ramBitCast<RamSigned>(b[0])) ? -1 : (ramBitCast<RamSigned>(a[0]) > ramBitCast<RamSigned>(b[0])) ? 1 :((ramBitCast<RamSigned>(a[1]) < ramBitCast<RamSigned>(b[1])) ? -1 : (ramBitCast<RamSigned>(a[1]) > ramBitCast<RamSigned>(b[1])) ? 1 :(0));
 }
bool less(const t_tuple& a, const t_tuple& b) const {
  return (ramBitCast<RamSigned>(a[0]) < ramBitCast<RamSigned>(b[0]))|| ((ramBitCast<RamSigned>(a[0]) == ramBitCast<RamSigned>(b[0])) && ((ramBitCast<RamSigned>(a[1]) < ramBitCast<RamSigned>(b[1]))));
 }
bool equal(const t_tuple& a, const t_tuple& b) const {
return (ramBitCast<RamSigned>(a[0]) == ramBitCast<RamSigned>(b[0]))&&(ramBitCast<RamSigned>(a[1]) == ramBitCast<RamSigned>(b[1]));
 }
};
using t_ind_0 = btree_set<t_tuple,t_comparator_0>;
t_ind_0 ind_0;
using iterator = t_ind_0::iterator;
struct context {
t_ind_0::operation_hints hints_0_lower;
t_ind_0::operation_hints hints_0_upper;
};
context createContext() { return context(); }
bool insert(const t_tuple& t);
bool insert(const t_tuple& t, context& h);
bool insert(const RamDomain* ramDomain);
bool insert(RamDomain a0,RamDomain a1);
bool contains(const t_tuple& t, context& h) const;
bool contains(const t_tuple& t) const;
std::size_t size() const;
iterator find(const t_tuple& t, context& h) const;
iterator find(const t_tuple& t) const;
range<iterator> lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */, context& /* h */) const;
range<iterator> lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */) const;
range<t_ind_0::iterator> lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper, context& h) const;
range<t_ind_0::iterator> lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper) const;
bool empty() const;
std::vector<range<iterator>> partition() const;
void purge();
iterator begin() const;
iterator end() const;
void printStatistics(std::ostream& o) const;
};
} // namespace souffle::t_btree_000_ii__0_1__11 
namespace souffle::t_btree_000_ii__0_1__11 {
using namespace souffle;
using t_ind_0 = Type::t_ind_0;
using iterator = Type::iterator;
using context = Type::context;
bool Type::insert(const t_tuple& t) {
context h;
return insert(t, h);
}
bool Type::insert(const t_tuple& t, context& h) {
if (ind_0.insert(t, h.hints_0_lower)) {
return true;
} else return false;
}
bool Type::insert(const RamDomain* ramDomain) {
RamDomain data[2];
std::copy(ramDomain, ramDomain + 2, data);
const t_tuple& tuple = reinterpret_cast<const t_tuple&>(data);
context h;
return insert(tuple, h);
}
bool Type::insert(RamDomain a0,RamDomain a1) {
RamDomain data[2] = {a0,a1};
return insert(data);
}
bool Type::contains(const t_tuple& t, context& h) const {
return ind_0.contains(t, h.hints_0_lower);
}
bool Type::contains(const t_tuple& t) const {
context h;
return contains(t, h);
}
std::size_t Type::size() const {
return ind_0.size();
}
iterator Type::find(const t_tuple& t, context& h) const {
return ind_0.find(t, h.hints_0_lower);
}
iterator Type::find(const t_tuple& t) const {
context h;
return find(t, h);
}
range<iterator> Type::lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */, context& /* h */) const {
return range<iterator>(ind_0.begin(),ind_0.end());
}
range<iterator> Type::lowerUpperRange_00(const t_tuple& /* lower */, const t_tuple& /* upper */) const {
return range<iterator>(ind_0.begin(),ind_0.end());
}
range<t_ind_0::iterator> Type::lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper, context& h) const {
t_comparator_0 comparator;
int cmp = comparator(lower, upper);
if (cmp == 0) {
    auto pos = ind_0.find(lower, h.hints_0_lower);
    auto fin = ind_0.end();
    if (pos != fin) {fin = pos; ++fin;}
    return make_range(pos, fin);
}
if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
}
return make_range(ind_0.lower_bound(lower, h.hints_0_lower), ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator> Type::lowerUpperRange_11(const t_tuple& lower, const t_tuple& upper) const {
context h;
return lowerUpperRange_11(lower,upper,h);
}
bool Type::empty() const {
return ind_0.empty();
}
std::vector<range<iterator>> Type::partition() const {
return ind_0.getChunks(400);
}
void Type::purge() {
ind_0.clear();
}
iterator Type::begin() const {
return ind_0.begin();
}
iterator Type::end() const {
return ind_0.end();
}
void Type::printStatistics(std::ostream& o) const {
o << " arity 2 direct b-tree index 0 lex-order [0,1]\n";
ind_0.printStats(o);
}
} // namespace souffle::t_btree_000_ii__0_1__11 
namespace  souffle {
using namespace souffle;
class Stratum_Alert_3bffae43927cf8ad {
public:
 Stratum_Alert_3bffae43927cf8ad(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_ii__0_1__11::Type& rel_Alert_2594a7302d7748af,t_btree_000_ii__0_1__11::Type& rel_Reach_da4ff294b7775935,t_btree_000_ii__1_0__11__01::Type& rel_Sink_4ef1896759d10549);
void run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret);
private:
SymbolTable& symTable;
RecordTable& recordTable;
ConcurrentCache<std::string,std::regex>& regexCache;
bool& pruneImdtRels;
bool& performIO;
SignalHandler*& signalHandler;
std::atomic<std::size_t>& iter;
std::atomic<RamDomain>& ctr;
std::string& inputDirectory;
std::string& outputDirectory;
t_btree_000_ii__0_1__11::Type* rel_Alert_2594a7302d7748af;
t_btree_000_ii__0_1__11::Type* rel_Reach_da4ff294b7775935;
t_btree_000_ii__1_0__11__01::Type* rel_Sink_4ef1896759d10549;
};
} // namespace  souffle
namespace  souffle {
using namespace souffle;
 Stratum_Alert_3bffae43927cf8ad::Stratum_Alert_3bffae43927cf8ad(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_ii__0_1__11::Type& rel_Alert_2594a7302d7748af,t_btree_000_ii__0_1__11::Type& rel_Reach_da4ff294b7775935,t_btree_000_ii__1_0__11__01::Type& rel_Sink_4ef1896759d10549):
symTable(symTable),
recordTable(recordTable),
regexCache(regexCache),
pruneImdtRels(pruneImdtRels),
performIO(performIO),
signalHandler(signalHandler),
iter(iter),
ctr(ctr),
inputDirectory(inputDirectory),
outputDirectory(outputDirectory),
rel_Alert_2594a7302d7748af(&rel_Alert_2594a7302d7748af),
rel_Reach_da4ff294b7775935(&rel_Reach_da4ff294b7775935),
rel_Sink_4ef1896759d10549(&rel_Sink_4ef1896759d10549){
}

void Stratum_Alert_3bffae43927cf8ad::run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret){
signalHandler->setMsg(R"_(Alert(s,k) :- 
   Reach(s,v),
   Sink(k,v).
in file taint.dl [32:1-32:40])_");
if(!(rel_Reach_da4ff294b7775935->empty()) && !(rel_Sink_4ef1896759d10549->empty())) {
[&](){
CREATE_OP_CONTEXT(rel_Alert_2594a7302d7748af_op_ctxt,rel_Alert_2594a7302d7748af->createContext());
CREATE_OP_CONTEXT(rel_Reach_da4ff294b7775935_op_ctxt,rel_Reach_da4ff294b7775935->createContext());
CREATE_OP_CONTEXT(rel_Sink_4ef1896759d10549_op_ctxt,rel_Sink_4ef1896759d10549->createContext());
for(const auto& env0 : *rel_Reach_da4ff294b7775935) {
auto range = rel_Sink_4ef1896759d10549->lowerUpperRange_01(Tuple<RamDomain,2>{{ramBitCast<RamDomain>(MIN_RAM_SIGNED), ramBitCast(env0[1])}},Tuple<RamDomain,2>{{ramBitCast<RamDomain>(MAX_RAM_SIGNED), ramBitCast(env0[1])}},READ_OP_CONTEXT(rel_Sink_4ef1896759d10549_op_ctxt));
for(const auto& env1 : range) {
Tuple<RamDomain,2> tuple{{ramBitCast(env0[0]),ramBitCast(env1[0])}};
rel_Alert_2594a7302d7748af->insert(tuple,READ_OP_CONTEXT(rel_Alert_2594a7302d7748af_op_ctxt));
}
}
}
();}
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(s	k)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(name)_",R"_(Alert)_"},{R"_(operation)_",R"_(output)_"},{R"_(output-dir)_",R"_(.)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 2, "params": ["s", "k"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 2, "types": ["i:number", "i:number"]}})_"}});
if (outputDirectory == "-"){directiveMap["IO"] = "stdout"; directiveMap["headers"] = "true";}
else if (!outputDirectory.empty()) {directiveMap["output-dir"] = outputDirectory;}
IOSystem::getInstance().getWriter(directiveMap, symTable, recordTable)->writeAll(*rel_Alert_2594a7302d7748af);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
if (pruneImdtRels) rel_Sink_4ef1896759d10549->purge();
}

} // namespace  souffle

namespace  souffle {
using namespace souffle;
class Stratum_Flow_50fb57361b491572 {
public:
 Stratum_Flow_50fb57361b491572(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_ii__0_1__11__10::Type& rel_Flow_b19aa17984410369);
void run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret);
private:
SymbolTable& symTable;
RecordTable& recordTable;
ConcurrentCache<std::string,std::regex>& regexCache;
bool& pruneImdtRels;
bool& performIO;
SignalHandler*& signalHandler;
std::atomic<std::size_t>& iter;
std::atomic<RamDomain>& ctr;
std::string& inputDirectory;
std::string& outputDirectory;
t_btree_000_ii__0_1__11__10::Type* rel_Flow_b19aa17984410369;
};
} // namespace  souffle
namespace  souffle {
using namespace souffle;
 Stratum_Flow_50fb57361b491572::Stratum_Flow_50fb57361b491572(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_ii__0_1__11__10::Type& rel_Flow_b19aa17984410369):
symTable(symTable),
recordTable(recordTable),
regexCache(regexCache),
pruneImdtRels(pruneImdtRels),
performIO(performIO),
signalHandler(signalHandler),
iter(iter),
ctr(ctr),
inputDirectory(inputDirectory),
outputDirectory(outputDirectory),
rel_Flow_b19aa17984410369(&rel_Flow_b19aa17984410369){
}

void Stratum_Flow_50fb57361b491572::run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret){
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(v	w)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(fact-dir)_",R"_(.)_"},{R"_(name)_",R"_(Flow)_"},{R"_(operation)_",R"_(input)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 2, "params": ["v", "w"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 2, "types": ["i:number", "i:number"]}})_"}});
if (!inputDirectory.empty()) {directiveMap["fact-dir"] = inputDirectory;}
IOSystem::getInstance().getReader(directiveMap, symTable, recordTable)->readAll(*rel_Flow_b19aa17984410369);
} catch (std::exception& e) {std::cerr << "Error loading Flow data: " << e.what() << '\n';
exit(1);
}
}
}

} // namespace  souffle

namespace  souffle {
using namespace souffle;
class Stratum_Reach_d86852c2f1aaf8eb {
public:
 Stratum_Reach_d86852c2f1aaf8eb(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_ii__0_1__11::Type& rel_delta_Reach_0320148c51832df9,t_btree_000_ii__0_1__11::Type& rel_new_Reach_12c3d29703df7222,t_btree_000_ii__0_1__11__10::Type& rel_Flow_b19aa17984410369,t_btree_000_ii__0_1__11::Type& rel_Reach_da4ff294b7775935,t_btree_000_i__0__1::Type& rel_Sanitized_560ce22b90f90168,t_btree_000_ii__0_1__11::Type& rel_Source_724b8cde435eac4d);
void run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret);
private:
SymbolTable& symTable;
RecordTable& recordTable;
ConcurrentCache<std::string,std::regex>& regexCache;
bool& pruneImdtRels;
bool& performIO;
SignalHandler*& signalHandler;
std::atomic<std::size_t>& iter;
std::atomic<RamDomain>& ctr;
std::string& inputDirectory;
std::string& outputDirectory;
t_btree_000_ii__0_1__11::Type* rel_delta_Reach_0320148c51832df9;
t_btree_000_ii__0_1__11::Type* rel_new_Reach_12c3d29703df7222;
t_btree_000_ii__0_1__11__10::Type* rel_Flow_b19aa17984410369;
t_btree_000_ii__0_1__11::Type* rel_Reach_da4ff294b7775935;
t_btree_000_i__0__1::Type* rel_Sanitized_560ce22b90f90168;
t_btree_000_ii__0_1__11::Type* rel_Source_724b8cde435eac4d;
};
} // namespace  souffle
namespace  souffle {
using namespace souffle;
 Stratum_Reach_d86852c2f1aaf8eb::Stratum_Reach_d86852c2f1aaf8eb(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_ii__0_1__11::Type& rel_delta_Reach_0320148c51832df9,t_btree_000_ii__0_1__11::Type& rel_new_Reach_12c3d29703df7222,t_btree_000_ii__0_1__11__10::Type& rel_Flow_b19aa17984410369,t_btree_000_ii__0_1__11::Type& rel_Reach_da4ff294b7775935,t_btree_000_i__0__1::Type& rel_Sanitized_560ce22b90f90168,t_btree_000_ii__0_1__11::Type& rel_Source_724b8cde435eac4d):
symTable(symTable),
recordTable(recordTable),
regexCache(regexCache),
pruneImdtRels(pruneImdtRels),
performIO(performIO),
signalHandler(signalHandler),
iter(iter),
ctr(ctr),
inputDirectory(inputDirectory),
outputDirectory(outputDirectory),
rel_delta_Reach_0320148c51832df9(&rel_delta_Reach_0320148c51832df9),
rel_new_Reach_12c3d29703df7222(&rel_new_Reach_12c3d29703df7222),
rel_Flow_b19aa17984410369(&rel_Flow_b19aa17984410369),
rel_Reach_da4ff294b7775935(&rel_Reach_da4ff294b7775935),
rel_Sanitized_560ce22b90f90168(&rel_Sanitized_560ce22b90f90168),
rel_Source_724b8cde435eac4d(&rel_Source_724b8cde435eac4d){
}

void Stratum_Reach_d86852c2f1aaf8eb::run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret){
signalHandler->setMsg(R"_(Reach(s,v) :- 
   Source(s,v).
in file taint.dl [30:1-30:29])_");
if(!(rel_Source_724b8cde435eac4d->empty())) {
[&](){
CREATE_OP_CONTEXT(rel_Reach_da4ff294b7775935_op_ctxt,rel_Reach_da4ff294b7775935->createContext());
CREATE_OP_CONTEXT(rel_Source_724b8cde435eac4d_op_ctxt,rel_Source_724b8cde435eac4d->createContext());
for(const auto& env0 : *rel_Source_724b8cde435eac4d) {
Tuple<RamDomain,2> tuple{{ramBitCast(env0[0]),ramBitCast(env0[1])}};
rel_Reach_da4ff294b7775935->insert(tuple,READ_OP_CONTEXT(rel_Reach_da4ff294b7775935_op_ctxt));
}
}
();}
[&](){
CREATE_OP_CONTEXT(rel_delta_Reach_0320148c51832df9_op_ctxt,rel_delta_Reach_0320148c51832df9->createContext());
CREATE_OP_CONTEXT(rel_Reach_da4ff294b7775935_op_ctxt,rel_Reach_da4ff294b7775935->createContext());
for(const auto& env0 : *rel_Reach_da4ff294b7775935) {
Tuple<RamDomain,2> tuple{{ramBitCast(env0[0]),ramBitCast(env0[1])}};
rel_delta_Reach_0320148c51832df9->insert(tuple,READ_OP_CONTEXT(rel_delta_Reach_0320148c51832df9_op_ctxt));
}
}
();auto loop_counter = RamUnsigned(1);
iter = 0;
for(;;) {
signalHandler->setMsg(R"_(Reach(s,w) :- 
   Reach(s,v),
   Flow(v,w),
   !Sanitized(w).
in file taint.dl [31:1-31:55])_");
if(!(rel_delta_Reach_0320148c51832df9->empty()) && !(rel_Flow_b19aa17984410369->empty())) {
[&](){
CREATE_OP_CONTEXT(rel_delta_Reach_0320148c51832df9_op_ctxt,rel_delta_Reach_0320148c51832df9->createContext());
CREATE_OP_CONTEXT(rel_new_Reach_12c3d29703df7222_op_ctxt,rel_new_Reach_12c3d29703df7222->createContext());
CREATE_OP_CONTEXT(rel_Flow_b19aa17984410369_op_ctxt,rel_Flow_b19aa17984410369->createContext());
CREATE_OP_CONTEXT(rel_Reach_da4ff294b7775935_op_ctxt,rel_Reach_da4ff294b7775935->createContext());
CREATE_OP_CONTEXT(rel_Sanitized_560ce22b90f90168_op_ctxt,rel_Sanitized_560ce22b90f90168->createContext());
for(const auto& env0 : *rel_delta_Reach_0320148c51832df9) {
auto range = rel_Flow_b19aa17984410369->lowerUpperRange_10(Tuple<RamDomain,2>{{ramBitCast(env0[1]), ramBitCast<RamDomain>(MIN_RAM_SIGNED)}},Tuple<RamDomain,2>{{ramBitCast(env0[1]), ramBitCast<RamDomain>(MAX_RAM_SIGNED)}},READ_OP_CONTEXT(rel_Flow_b19aa17984410369_op_ctxt));
for(const auto& env1 : range) {
if( !(rel_Sanitized_560ce22b90f90168->contains(Tuple<RamDomain,1>{{ramBitCast(env1[1])}},READ_OP_CONTEXT(rel_Sanitized_560ce22b90f90168_op_ctxt))) && !(rel_Reach_da4ff294b7775935->contains(Tuple<RamDomain,2>{{ramBitCast(env0[0]),ramBitCast(env1[1])}},READ_OP_CONTEXT(rel_Reach_da4ff294b7775935_op_ctxt)))) {
Tuple<RamDomain,2> tuple{{ramBitCast(env0[0]),ramBitCast(env1[1])}};
rel_new_Reach_12c3d29703df7222->insert(tuple,READ_OP_CONTEXT(rel_new_Reach_12c3d29703df7222_op_ctxt));
}
}
}
}
();}
if(rel_new_Reach_12c3d29703df7222->empty()) break;
[&](){
CREATE_OP_CONTEXT(rel_new_Reach_12c3d29703df7222_op_ctxt,rel_new_Reach_12c3d29703df7222->createContext());
CREATE_OP_CONTEXT(rel_Reach_da4ff294b7775935_op_ctxt,rel_Reach_da4ff294b7775935->createContext());
for(const auto& env0 : *rel_new_Reach_12c3d29703df7222) {
Tuple<RamDomain,2> tuple{{ramBitCast(env0[0]),ramBitCast(env0[1])}};
rel_Reach_da4ff294b7775935->insert(tuple,READ_OP_CONTEXT(rel_Reach_da4ff294b7775935_op_ctxt));
}
}
();std::swap(rel_delta_Reach_0320148c51832df9, rel_new_Reach_12c3d29703df7222);
rel_new_Reach_12c3d29703df7222->purge();
loop_counter = (ramBitCast<RamUnsigned>(loop_counter) + ramBitCast<RamUnsigned>(RamUnsigned(1)));
iter++;
}
iter = 0;
rel_delta_Reach_0320148c51832df9->purge();
rel_new_Reach_12c3d29703df7222->purge();
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(s	v)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(name)_",R"_(Reach)_"},{R"_(operation)_",R"_(output)_"},{R"_(output-dir)_",R"_(.)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 2, "params": ["s", "v"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 2, "types": ["i:number", "i:number"]}})_"}});
if (outputDirectory == "-"){directiveMap["IO"] = "stdout"; directiveMap["headers"] = "true";}
else if (!outputDirectory.empty()) {directiveMap["output-dir"] = outputDirectory;}
IOSystem::getInstance().getWriter(directiveMap, symTable, recordTable)->writeAll(*rel_Reach_da4ff294b7775935);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
if (pruneImdtRels) rel_Flow_b19aa17984410369->purge();
if (pruneImdtRels) rel_Sanitized_560ce22b90f90168->purge();
if (pruneImdtRels) rel_Source_724b8cde435eac4d->purge();
}

} // namespace  souffle

namespace  souffle {
using namespace souffle;
class Stratum_Sanitized_1f46b82438573c1c {
public:
 Stratum_Sanitized_1f46b82438573c1c(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_i__0__1::Type& rel_Sanitized_560ce22b90f90168);
void run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret);
private:
SymbolTable& symTable;
RecordTable& recordTable;
ConcurrentCache<std::string,std::regex>& regexCache;
bool& pruneImdtRels;
bool& performIO;
SignalHandler*& signalHandler;
std::atomic<std::size_t>& iter;
std::atomic<RamDomain>& ctr;
std::string& inputDirectory;
std::string& outputDirectory;
t_btree_000_i__0__1::Type* rel_Sanitized_560ce22b90f90168;
};
} // namespace  souffle
namespace  souffle {
using namespace souffle;
 Stratum_Sanitized_1f46b82438573c1c::Stratum_Sanitized_1f46b82438573c1c(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_i__0__1::Type& rel_Sanitized_560ce22b90f90168):
symTable(symTable),
recordTable(recordTable),
regexCache(regexCache),
pruneImdtRels(pruneImdtRels),
performIO(performIO),
signalHandler(signalHandler),
iter(iter),
ctr(ctr),
inputDirectory(inputDirectory),
outputDirectory(outputDirectory),
rel_Sanitized_560ce22b90f90168(&rel_Sanitized_560ce22b90f90168){
}

void Stratum_Sanitized_1f46b82438573c1c::run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret){
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(v)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(fact-dir)_",R"_(.)_"},{R"_(name)_",R"_(Sanitized)_"},{R"_(operation)_",R"_(input)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 1, "params": ["v"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 1, "types": ["i:number"]}})_"}});
if (!inputDirectory.empty()) {directiveMap["fact-dir"] = inputDirectory;}
IOSystem::getInstance().getReader(directiveMap, symTable, recordTable)->readAll(*rel_Sanitized_560ce22b90f90168);
} catch (std::exception& e) {std::cerr << "Error loading Sanitized data: " << e.what() << '\n';
exit(1);
}
}
}

} // namespace  souffle

namespace  souffle {
using namespace souffle;
class Stratum_Sink_db100583d7730eaa {
public:
 Stratum_Sink_db100583d7730eaa(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_ii__1_0__11__01::Type& rel_Sink_4ef1896759d10549);
void run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret);
private:
SymbolTable& symTable;
RecordTable& recordTable;
ConcurrentCache<std::string,std::regex>& regexCache;
bool& pruneImdtRels;
bool& performIO;
SignalHandler*& signalHandler;
std::atomic<std::size_t>& iter;
std::atomic<RamDomain>& ctr;
std::string& inputDirectory;
std::string& outputDirectory;
t_btree_000_ii__1_0__11__01::Type* rel_Sink_4ef1896759d10549;
};
} // namespace  souffle
namespace  souffle {
using namespace souffle;
 Stratum_Sink_db100583d7730eaa::Stratum_Sink_db100583d7730eaa(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_ii__1_0__11__01::Type& rel_Sink_4ef1896759d10549):
symTable(symTable),
recordTable(recordTable),
regexCache(regexCache),
pruneImdtRels(pruneImdtRels),
performIO(performIO),
signalHandler(signalHandler),
iter(iter),
ctr(ctr),
inputDirectory(inputDirectory),
outputDirectory(outputDirectory),
rel_Sink_4ef1896759d10549(&rel_Sink_4ef1896759d10549){
}

void Stratum_Sink_db100583d7730eaa::run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret){
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(k	v)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(fact-dir)_",R"_(.)_"},{R"_(name)_",R"_(Sink)_"},{R"_(operation)_",R"_(input)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 2, "params": ["k", "v"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 2, "types": ["i:number", "i:number"]}})_"}});
if (!inputDirectory.empty()) {directiveMap["fact-dir"] = inputDirectory;}
IOSystem::getInstance().getReader(directiveMap, symTable, recordTable)->readAll(*rel_Sink_4ef1896759d10549);
} catch (std::exception& e) {std::cerr << "Error loading Sink data: " << e.what() << '\n';
exit(1);
}
}
}

} // namespace  souffle

namespace  souffle {
using namespace souffle;
class Stratum_Source_ee2e7dc13ff2c53e {
public:
 Stratum_Source_ee2e7dc13ff2c53e(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_ii__0_1__11::Type& rel_Source_724b8cde435eac4d);
void run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret);
private:
SymbolTable& symTable;
RecordTable& recordTable;
ConcurrentCache<std::string,std::regex>& regexCache;
bool& pruneImdtRels;
bool& performIO;
SignalHandler*& signalHandler;
std::atomic<std::size_t>& iter;
std::atomic<RamDomain>& ctr;
std::string& inputDirectory;
std::string& outputDirectory;
t_btree_000_ii__0_1__11::Type* rel_Source_724b8cde435eac4d;
};
} // namespace  souffle
namespace  souffle {
using namespace souffle;
 Stratum_Source_ee2e7dc13ff2c53e::Stratum_Source_ee2e7dc13ff2c53e(SymbolTable& symTable,RecordTable& recordTable,ConcurrentCache<std::string,std::regex>& regexCache,bool& pruneImdtRels,bool& performIO,SignalHandler*& signalHandler,std::atomic<std::size_t>& iter,std::atomic<RamDomain>& ctr,std::string& inputDirectory,std::string& outputDirectory,t_btree_000_ii__0_1__11::Type& rel_Source_724b8cde435eac4d):
symTable(symTable),
recordTable(recordTable),
regexCache(regexCache),
pruneImdtRels(pruneImdtRels),
performIO(performIO),
signalHandler(signalHandler),
iter(iter),
ctr(ctr),
inputDirectory(inputDirectory),
outputDirectory(outputDirectory),
rel_Source_724b8cde435eac4d(&rel_Source_724b8cde435eac4d){
}

void Stratum_Source_ee2e7dc13ff2c53e::run([[maybe_unused]] const std::vector<RamDomain>& args,[[maybe_unused]] std::vector<RamDomain>& ret){
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(s	v)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(fact-dir)_",R"_(.)_"},{R"_(name)_",R"_(Source)_"},{R"_(operation)_",R"_(input)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 2, "params": ["s", "v"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 2, "types": ["i:number", "i:number"]}})_"}});
if (!inputDirectory.empty()) {directiveMap["fact-dir"] = inputDirectory;}
IOSystem::getInstance().getReader(directiveMap, symTable, recordTable)->readAll(*rel_Source_724b8cde435eac4d);
} catch (std::exception& e) {std::cerr << "Error loading Source data: " << e.what() << '\n';
exit(1);
}
}
}

} // namespace  souffle

namespace  souffle {
using namespace souffle;
class Sf_taint: public SouffleProgram {
public:
 Sf_taint();
 ~Sf_taint();
void run();
void runAll(std::string inputDirectoryArg = "",std::string outputDirectoryArg = "",bool performIOArg = true,bool pruneImdtRelsArg = true);
void printAll([[maybe_unused]] std::string outputDirectoryArg = "");
void loadAll([[maybe_unused]] std::string inputDirectoryArg = "");
void dumpInputs();
void dumpOutputs();
SymbolTable& getSymbolTable();
RecordTable& getRecordTable();
void setNumThreads(std::size_t numThreadsValue);
void executeSubroutine(std::string name,const std::vector<RamDomain>& args,std::vector<RamDomain>& ret);
private:
void runFunction(std::string inputDirectoryArg,std::string outputDirectoryArg,bool performIOArg,bool pruneImdtRelsArg);
SymbolTableImpl symTable;
SpecializedRecordTable<0> recordTable;
ConcurrentCache<std::string,std::regex> regexCache;
Own<t_btree_000_ii__0_1__11__10::Type> rel_Flow_b19aa17984410369;
souffle::RelationWrapper<t_btree_000_ii__0_1__11__10::Type> wrapper_rel_Flow_b19aa17984410369;
Own<t_btree_000_i__0__1::Type> rel_Sanitized_560ce22b90f90168;
souffle::RelationWrapper<t_btree_000_i__0__1::Type> wrapper_rel_Sanitized_560ce22b90f90168;
Own<t_btree_000_ii__1_0__11__01::Type> rel_Sink_4ef1896759d10549;
souffle::RelationWrapper<t_btree_000_ii__1_0__11__01::Type> wrapper_rel_Sink_4ef1896759d10549;
Own<t_btree_000_ii__0_1__11::Type> rel_Source_724b8cde435eac4d;
souffle::RelationWrapper<t_btree_000_ii__0_1__11::Type> wrapper_rel_Source_724b8cde435eac4d;
Own<t_btree_000_ii__0_1__11::Type> rel_Reach_da4ff294b7775935;
souffle::RelationWrapper<t_btree_000_ii__0_1__11::Type> wrapper_rel_Reach_da4ff294b7775935;
Own<t_btree_000_ii__0_1__11::Type> rel_new_Reach_12c3d29703df7222;
Own<t_btree_000_ii__0_1__11::Type> rel_delta_Reach_0320148c51832df9;
Own<t_btree_000_ii__0_1__11::Type> rel_Alert_2594a7302d7748af;
souffle::RelationWrapper<t_btree_000_ii__0_1__11::Type> wrapper_rel_Alert_2594a7302d7748af;
Stratum_Alert_3bffae43927cf8ad stratum_Alert_66e56e18f84e6e47;
Stratum_Flow_50fb57361b491572 stratum_Flow_bf66ee5bdd28c115;
Stratum_Reach_d86852c2f1aaf8eb stratum_Reach_0d15a6fa850f5dab;
Stratum_Sanitized_1f46b82438573c1c stratum_Sanitized_74f529f2666a2c6a;
Stratum_Sink_db100583d7730eaa stratum_Sink_d2631ffee02994a5;
Stratum_Source_ee2e7dc13ff2c53e stratum_Source_ad66afa194d6bfb9;
std::string inputDirectory;
std::string outputDirectory;
SignalHandler* signalHandler{SignalHandler::instance()};
std::atomic<RamDomain> ctr{};
std::atomic<std::size_t> iter{};
};
} // namespace  souffle
namespace  souffle {
using namespace souffle;
 Sf_taint::Sf_taint():
symTable(),
recordTable(),
regexCache(),
rel_Flow_b19aa17984410369(mk<t_btree_000_ii__0_1__11__10::Type>()),
wrapper_rel_Flow_b19aa17984410369(0, *rel_Flow_b19aa17984410369, *this, "Flow", std::array<const char *,2>{{"i:number","i:number"}}, std::array<const char *,2>{{"v","w"}}, 0),
rel_Sanitized_560ce22b90f90168(mk<t_btree_000_i__0__1::Type>()),
wrapper_rel_Sanitized_560ce22b90f90168(1, *rel_Sanitized_560ce22b90f90168, *this, "Sanitized", std::array<const char *,1>{{"i:number"}}, std::array<const char *,1>{{"v"}}, 0),
rel_Sink_4ef1896759d10549(mk<t_btree_000_ii__1_0__11__01::Type>()),
wrapper_rel_Sink_4ef1896759d10549(2, *rel_Sink_4ef1896759d10549, *this, "Sink", std::array<const char *,2>{{"i:number","i:number"}}, std::array<const char *,2>{{"k","v"}}, 0),
rel_Source_724b8cde435eac4d(mk<t_btree_000_ii__0_1__11::Type>()),
wrapper_rel_Source_724b8cde435eac4d(3, *rel_Source_724b8cde435eac4d, *this, "Source", std::array<const char *,2>{{"i:number","i:number"}}, std::array<const char *,2>{{"s","v"}}, 0),
rel_Reach_da4ff294b7775935(mk<t_btree_000_ii__0_1__11::Type>()),
wrapper_rel_Reach_da4ff294b7775935(4, *rel_Reach_da4ff294b7775935, *this, "Reach", std::array<const char *,2>{{"i:number","i:number"}}, std::array<const char *,2>{{"s","v"}}, 0),
rel_new_Reach_12c3d29703df7222(mk<t_btree_000_ii__0_1__11::Type>()),
rel_delta_Reach_0320148c51832df9(mk<t_btree_000_ii__0_1__11::Type>()),
rel_Alert_2594a7302d7748af(mk<t_btree_000_ii__0_1__11::Type>()),
wrapper_rel_Alert_2594a7302d7748af(5, *rel_Alert_2594a7302d7748af, *this, "Alert", std::array<const char *,2>{{"i:number","i:number"}}, std::array<const char *,2>{{"s","k"}}, 0),
stratum_Alert_66e56e18f84e6e47(symTable,recordTable,regexCache,pruneImdtRels,performIO,signalHandler,iter,ctr,inputDirectory,outputDirectory,*rel_Alert_2594a7302d7748af,*rel_Reach_da4ff294b7775935,*rel_Sink_4ef1896759d10549),
stratum_Flow_bf66ee5bdd28c115(symTable,recordTable,regexCache,pruneImdtRels,performIO,signalHandler,iter,ctr,inputDirectory,outputDirectory,*rel_Flow_b19aa17984410369),
stratum_Reach_0d15a6fa850f5dab(symTable,recordTable,regexCache,pruneImdtRels,performIO,signalHandler,iter,ctr,inputDirectory,outputDirectory,*rel_delta_Reach_0320148c51832df9,*rel_new_Reach_12c3d29703df7222,*rel_Flow_b19aa17984410369,*rel_Reach_da4ff294b7775935,*rel_Sanitized_560ce22b90f90168,*rel_Source_724b8cde435eac4d),
stratum_Sanitized_74f529f2666a2c6a(symTable,recordTable,regexCache,pruneImdtRels,performIO,signalHandler,iter,ctr,inputDirectory,outputDirectory,*rel_Sanitized_560ce22b90f90168),
stratum_Sink_d2631ffee02994a5(symTable,recordTable,regexCache,pruneImdtRels,performIO,signalHandler,iter,ctr,inputDirectory,outputDirectory,*rel_Sink_4ef1896759d10549),
stratum_Source_ad66afa194d6bfb9(symTable,recordTable,regexCache,pruneImdtRels,performIO,signalHandler,iter,ctr,inputDirectory,outputDirectory,*rel_Source_724b8cde435eac4d){
addRelation("Flow", wrapper_rel_Flow_b19aa17984410369, true, false);
addRelation("Sanitized", wrapper_rel_Sanitized_560ce22b90f90168, true, false);
addRelation("Sink", wrapper_rel_Sink_4ef1896759d10549, true, false);
addRelation("Source", wrapper_rel_Source_724b8cde435eac4d, true, false);
addRelation("Reach", wrapper_rel_Reach_da4ff294b7775935, false, true);
addRelation("Alert", wrapper_rel_Alert_2594a7302d7748af, false, true);
}

 Sf_taint::~Sf_taint(){
}

void Sf_taint::runFunction(std::string inputDirectoryArg,std::string outputDirectoryArg,bool performIOArg,bool pruneImdtRelsArg){

    this->inputDirectory  = std::move(inputDirectoryArg);
    this->outputDirectory = std::move(outputDirectoryArg);
    this->performIO       = performIOArg;
    this->pruneImdtRels   = pruneImdtRelsArg;

    // set default threads (in embedded mode)
    // if this is not set, and omp is used, the default omp setting of number of cores is used.
#if defined(_OPENMP)
    if (0 < getNumThreads()) { omp_set_num_threads(static_cast<int>(getNumThreads())); }
#endif

    signalHandler->set();
// -- query evaluation --
{
 std::vector<RamDomain> args, ret;
stratum_Flow_bf66ee5bdd28c115.run(args, ret);
}
{
 std::vector<RamDomain> args, ret;
stratum_Sanitized_74f529f2666a2c6a.run(args, ret);
}
{
 std::vector<RamDomain> args, ret;
stratum_Sink_d2631ffee02994a5.run(args, ret);
}
{
 std::vector<RamDomain> args, ret;
stratum_Source_ad66afa194d6bfb9.run(args, ret);
}
{
 std::vector<RamDomain> args, ret;
stratum_Reach_0d15a6fa850f5dab.run(args, ret);
}
{
 std::vector<RamDomain> args, ret;
stratum_Alert_66e56e18f84e6e47.run(args, ret);
}

// -- relation hint statistics --
signalHandler->reset();
}

void Sf_taint::run(){
runFunction("", "", false, false);
}

void Sf_taint::runAll(std::string inputDirectoryArg,std::string outputDirectoryArg,bool performIOArg,bool pruneImdtRelsArg){
runFunction(inputDirectoryArg, outputDirectoryArg, performIOArg, pruneImdtRelsArg);
}

void Sf_taint::printAll([[maybe_unused]] std::string outputDirectoryArg){
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(s	v)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(name)_",R"_(Reach)_"},{R"_(operation)_",R"_(output)_"},{R"_(output-dir)_",R"_(.)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 2, "params": ["s", "v"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 2, "types": ["i:number", "i:number"]}})_"}});
if (!outputDirectoryArg.empty()) {directiveMap["output-dir"] = outputDirectoryArg;}
IOSystem::getInstance().getWriter(directiveMap, symTable, recordTable)->writeAll(*rel_Reach_da4ff294b7775935);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(s	k)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(name)_",R"_(Alert)_"},{R"_(operation)_",R"_(output)_"},{R"_(output-dir)_",R"_(.)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 2, "params": ["s", "k"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 2, "types": ["i:number", "i:number"]}})_"}});
if (!outputDirectoryArg.empty()) {directiveMap["output-dir"] = outputDirectoryArg;}
IOSystem::getInstance().getWriter(directiveMap, symTable, recordTable)->writeAll(*rel_Alert_2594a7302d7748af);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}

void Sf_taint::loadAll([[maybe_unused]] std::string inputDirectoryArg){
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(v	w)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(fact-dir)_",R"_(.)_"},{R"_(name)_",R"_(Flow)_"},{R"_(operation)_",R"_(input)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 2, "params": ["v", "w"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 2, "types": ["i:number", "i:number"]}})_"}});
if (!inputDirectoryArg.empty()) {directiveMap["fact-dir"] = inputDirectoryArg;}
IOSystem::getInstance().getReader(directiveMap, symTable, recordTable)->readAll(*rel_Flow_b19aa17984410369);
} catch (std::exception& e) {std::cerr << "Error loading Flow data: " << e.what() << '\n';
exit(1);
}
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(v)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(fact-dir)_",R"_(.)_"},{R"_(name)_",R"_(Sanitized)_"},{R"_(operation)_",R"_(input)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 1, "params": ["v"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 1, "types": ["i:number"]}})_"}});
if (!inputDirectoryArg.empty()) {directiveMap["fact-dir"] = inputDirectoryArg;}
IOSystem::getInstance().getReader(directiveMap, symTable, recordTable)->readAll(*rel_Sanitized_560ce22b90f90168);
} catch (std::exception& e) {std::cerr << "Error loading Sanitized data: " << e.what() << '\n';
exit(1);
}
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(k	v)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(fact-dir)_",R"_(.)_"},{R"_(name)_",R"_(Sink)_"},{R"_(operation)_",R"_(input)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 2, "params": ["k", "v"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 2, "types": ["i:number", "i:number"]}})_"}});
if (!inputDirectoryArg.empty()) {directiveMap["fact-dir"] = inputDirectoryArg;}
IOSystem::getInstance().getReader(directiveMap, symTable, recordTable)->readAll(*rel_Sink_4ef1896759d10549);
} catch (std::exception& e) {std::cerr << "Error loading Sink data: " << e.what() << '\n';
exit(1);
}
try {std::map<std::string, std::string> directiveMap({{R"_(IO)_",R"_(file)_"},{R"_(attributeNames)_",R"_(s	v)_"},{R"_(auxArity)_",R"_(0)_"},{R"_(fact-dir)_",R"_(.)_"},{R"_(name)_",R"_(Source)_"},{R"_(operation)_",R"_(input)_"},{R"_(params)_",R"_({"records": {}, "relation": {"arity": 2, "params": ["s", "v"]}})_"},{R"_(types)_",R"_({"ADTs": {}, "records": {}, "relation": {"arity": 2, "types": ["i:number", "i:number"]}})_"}});
if (!inputDirectoryArg.empty()) {directiveMap["fact-dir"] = inputDirectoryArg;}
IOSystem::getInstance().getReader(directiveMap, symTable, recordTable)->readAll(*rel_Source_724b8cde435eac4d);
} catch (std::exception& e) {std::cerr << "Error loading Source data: " << e.what() << '\n';
exit(1);
}
}

void Sf_taint::dumpInputs(){
try {std::map<std::string, std::string> rwOperation;
rwOperation["IO"] = "stdout";
rwOperation["name"] = "Flow";
rwOperation["types"] = R"_({"relation": {"arity": 2, "auxArity": 0, "types": ["i:number", "i:number"]}})_";
IOSystem::getInstance().getWriter(rwOperation, symTable, recordTable)->writeAll(*rel_Flow_b19aa17984410369);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
try {std::map<std::string, std::string> rwOperation;
rwOperation["IO"] = "stdout";
rwOperation["name"] = "Sanitized";
rwOperation["types"] = R"_({"relation": {"arity": 1, "auxArity": 0, "types": ["i:number"]}})_";
IOSystem::getInstance().getWriter(rwOperation, symTable, recordTable)->writeAll(*rel_Sanitized_560ce22b90f90168);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
try {std::map<std::string, std::string> rwOperation;
rwOperation["IO"] = "stdout";
rwOperation["name"] = "Sink";
rwOperation["types"] = R"_({"relation": {"arity": 2, "auxArity": 0, "types": ["i:number", "i:number"]}})_";
IOSystem::getInstance().getWriter(rwOperation, symTable, recordTable)->writeAll(*rel_Sink_4ef1896759d10549);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
try {std::map<std::string, std::string> rwOperation;
rwOperation["IO"] = "stdout";
rwOperation["name"] = "Source";
rwOperation["types"] = R"_({"relation": {"arity": 2, "auxArity": 0, "types": ["i:number", "i:number"]}})_";
IOSystem::getInstance().getWriter(rwOperation, symTable, recordTable)->writeAll(*rel_Source_724b8cde435eac4d);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}

void Sf_taint::dumpOutputs(){
try {std::map<std::string, std::string> rwOperation;
rwOperation["IO"] = "stdout";
rwOperation["name"] = "Reach";
rwOperation["types"] = R"_({"relation": {"arity": 2, "auxArity": 0, "types": ["i:number", "i:number"]}})_";
IOSystem::getInstance().getWriter(rwOperation, symTable, recordTable)->writeAll(*rel_Reach_da4ff294b7775935);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
try {std::map<std::string, std::string> rwOperation;
rwOperation["IO"] = "stdout";
rwOperation["name"] = "Alert";
rwOperation["types"] = R"_({"relation": {"arity": 2, "auxArity": 0, "types": ["i:number", "i:number"]}})_";
IOSystem::getInstance().getWriter(rwOperation, symTable, recordTable)->writeAll(*rel_Alert_2594a7302d7748af);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}

SymbolTable& Sf_taint::getSymbolTable(){
return symTable;
}

RecordTable& Sf_taint::getRecordTable(){
return recordTable;
}

void Sf_taint::setNumThreads(std::size_t numThreadsValue){
SouffleProgram::setNumThreads(numThreadsValue);
symTable.setNumLanes(getNumThreads());
recordTable.setNumLanes(getNumThreads());
regexCache.setNumLanes(getNumThreads());
}

void Sf_taint::executeSubroutine(std::string name,const std::vector<RamDomain>& args,std::vector<RamDomain>& ret){
if (name == "Alert") {
stratum_Alert_66e56e18f84e6e47.run(args, ret);
return;}
if (name == "Flow") {
stratum_Flow_bf66ee5bdd28c115.run(args, ret);
return;}
if (name == "Reach") {
stratum_Reach_0d15a6fa850f5dab.run(args, ret);
return;}
if (name == "Sanitized") {
stratum_Sanitized_74f529f2666a2c6a.run(args, ret);
return;}
if (name == "Sink") {
stratum_Sink_d2631ffee02994a5.run(args, ret);
return;}
if (name == "Source") {
stratum_Source_ad66afa194d6bfb9.run(args, ret);
return;}
fatal(("unknown subroutine " + name).c_str());
}

} // namespace  souffle
namespace souffle {
SouffleProgram *newInstance_taint(){return new  souffle::Sf_taint;}
SymbolTable *getST_taint(SouffleProgram *p){return &reinterpret_cast<souffle::Sf_taint*>(p)->getSymbolTable();}
} // namespace souffle

#ifndef __EMBEDDED_SOUFFLE__
#include "souffle/CompiledOptions.h"
int main(int argc, char** argv)
{
try{
souffle::CmdOptions opt(R"_(rules/taint.dl)_",
R"_()_",
R"_()_",
false,
R"_()_",
1);
if (!opt.parse(argc,argv)) return 1;
souffle::Sf_taint obj;
#if defined(_OPENMP) 
obj.setNumThreads(opt.getNumJobs());

#endif
obj.runAll(opt.getInputFileDir(), opt.getOutputFileDir());
return 0;
} catch(std::exception &e) { souffle::SignalHandler::instance()->error(e.what());}
}
#endif

namespace  souffle {
using namespace souffle;
class factory_Sf_taint: souffle::ProgramFactory {
public:
souffle::SouffleProgram* newInstance();
 factory_Sf_taint();
private:
};
} // namespace  souffle
namespace  souffle {
using namespace souffle;
souffle::SouffleProgram* factory_Sf_taint::newInstance(){
return new  souffle::Sf_taint();
}

 factory_Sf_taint::factory_Sf_taint():
souffle::ProgramFactory("taint"){
}

} // namespace  souffle
namespace souffle {

#ifdef __EMBEDDED_SOUFFLE__
extern "C" {
souffle::factory_Sf_taint __factory_Sf_taint_instance;
}
#endif
} // namespace souffle

