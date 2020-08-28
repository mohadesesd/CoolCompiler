#ifndef SEMANT_H_
#define SEMANT_H_
 
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"
#include "utilities.h"

class ClassTable;
typedef ClassTable* ClassTableP;

class ClassTable {
public:
    ClassTable(Classes classes);
    ostream& error_stream;
    int semant_errors;
    void register_basic_classes();
    ostream& semant_error();
    ostream& semant_error(tree_node *t);
    ostream& internal_error(int lineno);
    void register_classes(Classes classes);
    void register_methods();
    std::vector<Class_> getInheritancePath(Class_ c);
    std::vector<Class_> getInheritancePath(Symbol name);
    bool is_conform_to(Symbol name1, Symbol name2);
    Class_ LCA(Symbol name1, Symbol name2);
    method_class* getMethod(Class_ c, Symbol method_name);
    void check_inheritance();
    void check_main();
    void check_methods();
};



#define TRUE 1
#define FALSE 0

#endif

