#include "semant.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "utilities.h"


extern int semant_debug;
extern char *curr_filename;
static Class_ curr_class = 0;
typedef std::map<Symbol, Class_> Class_Table; // name, Class_
typedef SymbolTable<Symbol, Symbol> ObjectEnvironment; // name, type
typedef std::vector<method_class*> Methods;
typedef std:: map<Class_, Methods> MethodTable;
MethodTable methodTable;
Class_Table classTable;
ObjectEnvironment objectEnv;
static ClassTable* classtable;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////

static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;

//
// Initializing the predefined symbols.
//

static void initialize_constants(void) {
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}
ClassTable::ClassTable(Classes classes): semant_errors(0)
				       , error_stream(cerr)
{
}

void ClassTable::register_basic_classes(void) {
    // The tree package uses these globals to annotate the classes built below.
    // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);

    classTable[Object_class->getName()] = Object_class;
    classTable[IO_class->getName()] = IO_class;
    classTable[Int_class->getName()] = Int_class;
    classTable[Bool_class->getName()] = Bool_class;
    classTable[Str_class->getName()] = Str_class;
}

///////////////////////////////////////////////////////////
// Register all Classes of the progrma in Class Table 
// We cannot use SELF_TYPE for the name of a Class
// We cannot Redefine a Class that is already in Class Table
// We cannot inherit from Bool, Int, Str and also SELF_TYPE
////////////////////////////////////////////////////////

void ClassTable::register_classes(Classes classes) {
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {

        curr_class = classes->nth(i);
        Symbol parent_name = curr_class->getParentName();

        if (curr_class->getName() == SELF_TYPE)
            semant_error(curr_class) << "Redefinition of basic class SELF_TYPE.\n";

        else if (classTable.find(curr_class->getName()) != classTable.end())
            semant_error(curr_class) << "Class " << curr_class->getName() << " was previously defined.\n";

        else if (parent_name == Int || parent_name == Str || parent_name == Bool || parent_name == SELF_TYPE)
            semant_error(curr_class) << "Class " << curr_class->getName() << " cannot inherit class " << parent_name << ".\n";

        else
            classTable[curr_class->getName()] = curr_class;
    }
}

//////////////////////////////////////////////////////////////////
//Register Methods
//We cannot Redefine an existing method
/////////////////////////////////////////////////////////////////

void ClassTable::register_methods() {
    for (Class_Table::iterator it = classTable.begin(); it != classTable.end(); it++) {

        Features features = it->second->getFeatures();
        Methods methods;

        for (int i = features->first(); features->more(i); i = features->next(i))
            if (features->nth(i)->isMethod()) {

                method_class* method = static_cast<method_class*>(features->nth(i));
                
                bool existed = false;
                for (size_t j = 0; j < methods.size(); j++)
                    if (methods[j]->getName() == method->getName())
                        existed = true;
               
                if (existed)
                    semant_error(method) << "Method " << method->getName() << " is multiply defined.\n";

                else
                    methods.push_back(static_cast<method_class*>(features->nth(i)));
            }
        methodTable[it->second] = methods;
    }
}

/////////////////////////////////////////////////////////////////
//Get the Inheritance Path from "Object" to Class In Inheritance Tree
//child cannot Inherits from unregistered parent
////////////////////////////////////////////////////////////////

std::vector<Class_> ClassTable::getInheritancePath(Class_ class_object) {
    std::vector<Class_> class_inh_path;

    while (class_object->getName() != Object) {

        class_inh_path.push_back(class_object);

        if (classTable.find(class_object->getParentName()) == classTable.end())
            internal_error(__LINE__) << "invalid inheritance chain.\n";

        else
            class_object = classTable[class_object->getParentName()];
    }

    class_inh_path.push_back(classTable[Object]);
    return class_inh_path;
}

std::vector<Class_> ClassTable::getInheritancePath(Symbol class_name) {

    if (class_name == SELF_TYPE)
        class_name = curr_class->getName();

    if (classTable.find(class_name) == classTable.end())
        internal_error(__LINE__) << class_name << " not found in class table.\n";

    return getInheritancePath(classTable[class_name]);
}

////////////////////////////////////////////////////
// is_conform_to method check that if first type is  
// conform to the second type(i.e if the second type is one of
// first type ancestors)
// each class is conform to itself
// we cannot check the conformtion of undefined classes
///////////////////////////////////////////////////

bool ClassTable::is_conform_to(Symbol type1, Symbol type2) {

    if (type1 == SELF_TYPE && type2 == SELF_TYPE)
        return true;

    if (type1 != SELF_TYPE && type2 == SELF_TYPE)
        return false;

    if (type1 == SELF_TYPE)
        type1 = curr_class->getName();

    if (classTable.find(type1) == classTable.end())
        internal_error(__LINE__) << type1 << " not found in class table.\n";

    if (classTable.find(type2) == classTable.end())
        internal_error(__LINE__) << type2 << " not found in class table.\n";
    
    Class_ type1_obj = classTable[type1];
    Class_ type2_obj = classTable[type2];

    std::vector<Class_> class1_inh_path = getInheritancePath(type1_obj);
    
    for (size_t i = 0; i < class1_inh_path.size(); i++)
        if (class1_inh_path[i] == type2_obj)
            return true;

    return false;
}

////////////////////////////////////////////////////////
//Find the lowest common ancestor in inheritance tree(i.e join operation of two type)
///////////////////////////////////////////////////////

Class_ ClassTable::LCA(Symbol type1, Symbol type2) {

    std::vector<Class_> type1_inh_path = getInheritancePath(type1);
    std::vector<Class_> type2_inh_path = getInheritancePath(type2);

    std::reverse(type1_inh_path.begin(), type1_inh_path.end());
    std::reverse(type2_inh_path.begin(), type2_inh_path.end());

    size_t i;
    for (i = 1; i < std::min(type1_inh_path.size(), type2_inh_path.size()); i++)

        if (type1_inh_path[i] != type2_inh_path[i])
            return type1_inh_path[i - 1];

    return type1_inh_path[i - 1];
}

//////////////////////////////////////////////////////////////
//Get the special method of special Class
/////////////////////////////////////////////////////////////

method_class* ClassTable::getMethod(Class_ class_obj, Symbol method_name) {
    Methods methods = methodTable[class_obj];

    for (size_t i = 0; i < methods.size(); i++)
        if (methods[i]->getName() == method_name)
            return methods[i];

    return 0;
}

ostream& ClassTable::semant_error() {
    semant_errors++;
    return error_stream;
}

ostream& ClassTable::semant_error(tree_node *t) {
    error_stream << curr_class->getFileName() << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::internal_error(int lineno) {
    error_stream << "FATAL:" << lineno << ": ";
    return error_stream;
}
///////////////////////////////////////////////////////////
//In Cool we have no Inheritance Cycle
//We cannot inherit from undefined parent
///////////////////////////////////////////////////////////

void ClassTable::check_inheritance() {

    for (Class_Table::iterator it = classTable.begin(); it != classTable.end(); it++)

        if (it->first != Object && classTable.find(it->second->getParentName()) == classTable.end()) {
            curr_class = it->second;
            semant_error(curr_class) << "Class " << it->second->getName() << " inherits from an undefined class " << it->second->getParentName() << ".\n";
        }
    
    for (Class_Table::iterator it = classTable.begin(); it != classTable.end(); it++) {

        if (it->first == Object) continue;

        curr_class = it->second;

        Symbol class_name = it->first;
        Symbol parent_name = it->second->getParentName();

        while (parent_name != Object) {
            if (parent_name == class_name) {
                semant_error(curr_class) << "Class " << curr_class->getName() << ", or an ancestor of " << curr_class->getName() << ", is involved in an inheritance cycle.\n";
                break;
            }

            if (classTable.find(parent_name) == classTable.end())
                break;

            parent_name = classTable[parent_name]->getParentName();
        }
    }
}

/////////////////////////////////////
//Each Cool Program has a Main class with a main method with no argument
////////////////////////////////////

void ClassTable::check_main() {
    if (classTable.find(Main) == classTable.end()) {
        semant_error() << "Class Main is not defined.\n";
        return;
    }

    curr_class = classTable[Main];
    Features features = curr_class->getFeatures();

    bool find_main = false;
    for (int i = features->first(); features->more(i); i = features->next(i))
        if (features->nth(i)->isMethod() && static_cast<method_class*>(features->nth(i))->getName() == main_meth)
            find_main = true;
    
    if (!find_main)
        semant_error(curr_class) << "No 'main' method in class Main.\n";
}

/////////////////////////////////////////
//Overrided Inherited Methods should have same return type and same number of formals with same types
//each class attribute should has a defined type(a type that is present in Class Table) 
//and expr type should be conform to the type of the attribute
////////////////////////////////////////

void ClassTable::check_methods() {

    for (Class_Table::iterator it = classTable.begin(); it != classTable.end(); it++) {

        if (it->first == Object || it->first == IO || it->first == Int || it->first == Bool || it->first == Str) continue;
        Symbol class_name = it->first;
        curr_class = it->second; 

        std::vector<Class_> class_inh_path = getInheritancePath(curr_class);
        
	class_inh_path.push_back(curr_class);

        for (size_t t = 1; t < class_inh_path.size(); t++) {

            Class_ ancestor_class = class_inh_path[t];
            Features ancestor_features = ancestor_class->getFeatures();

            objectEnv.enterscope();

            for (int i = ancestor_features->first(); ancestor_features->more(i); i = ancestor_features->next(i)) {
                if (!ancestor_features->nth(i)->isAttr()) continue;
                attr_class* attr = static_cast<attr_class*>(ancestor_features->nth(i));
                objectEnv.addid(attr->getName(), new Symbol(attr->getType()));
            }
        }

        Features features = curr_class->getFeatures();

        for (int i = features->first(); features->more(i); i = features->next(i))

            if (features->nth(i)->isMethod()) {
                method_class* curr_method = static_cast<method_class*>(features->nth(i));

                curr_method->checkType();

                for (size_t k = 1; k < class_inh_path.size(); k++) {
                    method_class* ancestor_method = getMethod(class_inh_path[k], curr_method->getName());
                    if (!ancestor_method) continue;

                    if (curr_method->getReturnType() != ancestor_method->getReturnType())
                        semant_error(curr_method) << "In redefined method " << curr_method->getName() << ", return type " << curr_method->getReturnType() << " is different from original return type " << ancestor_method->getReturnType() << ".\n";

                    Formals curr_formals = curr_method->getFormals();
                    Formals ancestor_formals = ancestor_method->getFormals();

                    int it1 = curr_formals->first(), it2 = ancestor_formals->first();
                    while (curr_formals->more(it1) && ancestor_formals->more(it2)) {

                        if (curr_formals->nth(it1)->getType() != ancestor_formals->nth(it2)->getType())

                            semant_error(curr_formals->nth(it1)) << "In redefined method " << curr_method->getName() << ", parameter type " << curr_formals->nth(it1)->getType() << " is different from original type " << ancestor_formals->nth(it2)->getType() << ".\n";

                        it1 = curr_formals->next(it1);
                        it2 = ancestor_formals->next(it2);

                        if (curr_formals->more(it1) xor ancestor_formals->more(it2))
                            semant_error(curr_method) << "Incompatible number of formal parameters in redefined method " << curr_method->getName() << ".\n";
                    }
                }
            } else { // isAttr
                attr_class* curr_attr = static_cast<attr_class*>(features->nth(i));
                Symbol expr_type = curr_attr->getInitExpr()->checkType();

                if (classTable.find(curr_attr->getType()) == classTable.end())

                    semant_error(curr_attr) << "Class " << curr_attr->getType() << " of attribute " << curr_attr->getName() << " is undefined.\n";

                else if (classTable.find(expr_type) != classTable.end() && !is_conform_to(expr_type, curr_attr->getType()))
                    semant_error(curr_attr) << "Inferred type " << expr_type << " of initialization of attribute " << curr_attr->getName() << " does not conform to declared type " << curr_attr->getType() << ".\n";
            }

        for (size_t k = 1; k < class_inh_path.size(); k++)
            objectEnv.exitscope();
    }
}

///////////////////////////////////////////////
//we should check the types of method's formals and their name
//their type should be in the class table
//we can not define a same name formal multiple times
//we should check the expression type and its conformation to return type
//////////////////////////////////////////////

void method_class::checkType() {
    objectEnv.enterscope();

    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        
        if (formals->nth(i)->getName() == self)
           classtable->semant_error(formals->nth(i)) << "'self' cannot be the name of a formal parameter.\n";
        
	else if (objectEnv.lookup(formals->nth(i)->getName()))
            classtable->semant_error(formals->nth(i)) << "Formal parameter " << formals->nth(i)->getName() << " is multiply defined.\n";
        
	else if (classTable.find(formals->nth(i)->getType()) == classTable.end())
            classtable->semant_error(formals->nth(i)) << "Class " << formals->nth(i)->getType() << " of formal parameter " << formals->nth(i)->getName() << " is undefined.\n";
        
	else
            objectEnv.addid(formals->nth(i)->getName(), new Symbol(formals->nth(i)->getType()));
    }

    Symbol expr_type = expr->checkType();

    if (return_type != SELF_TYPE && classTable.find(return_type) == classTable.end())
        classtable->semant_error(this) << "Undefined return type " << return_type << " in method " << name << ".\n";

    else if (!classtable->is_conform_to(expr_type, return_type))
       classtable->semant_error(this) << "Inferred return type " << expr_type << " of method " << name << " does not conform to declared return type " << return_type << ".\n";

    objectEnv.exitscope();
}
//////////////////////////////////////////////////////
// Check the type of left and right and check the conformation of them.
// return the right type(expr type)
/////////////////////////////////////////////////////
Symbol assign_class::checkType() {
    Symbol rtype = expr->checkType();
    
    if (objectEnv.lookup(name) == 0) {
        classtable->semant_error(this) << "Assignment to undeclared variable " << name << ".\n";
        type = rtype;
        return type;
    }

    Symbol ltype = *objectEnv.lookup(name);
    
    if (!classtable->is_conform_to(rtype, ltype)) {
        classtable->semant_error(this) << "Type " << rtype << " of assigned expression does not conform to declared type " << ltype << " of identifier " << name << ".\n";
        type = ltype;
        return type;
    }

    type = rtype;
    return type;
}
//////////////////////////////////////////////////////////
//Check that if the type_name exist in the Class Table or not
//Check that if the method name exist in features of the class or its ancestor or not
//Check about the formals and actuals(their Type and their numbers)
/////////////////////////////////////////////////////////
Symbol static_dispatch_class::checkType() {
    bool error = false;
    Symbol expr_type = expr->checkType();

    if (this->type_name != SELF_TYPE && classTable.find(this->type_name) == classTable.end()) {
        classtable->semant_error(this) << "Static dispatch to undefined class " << this->type_name << ".\n";
        type = Object;
        return type;
    }

    if (expr_type != SELF_TYPE && classTable.find(expr_type) == classTable.end()) {
        type = Object;
        return type;
    }

    if (!classtable->is_conform_to(expr_type, this->type_name)) {
        error = true;
        classtable->semant_error(this) << "Expression type " << expr_type << " does not conform to declared static dispatch type " << this->type_name << ".\n";
    }
    bool flag = false;
    method_class* method = 0;
    std::vector<Class_> class_inh_path = (classtable->getInheritancePath(this->type_name));
    for (size_t i = 0; i < class_inh_path.size(); i++) {
        Methods methods = methodTable[class_inh_path[i]];
        for (size_t j = 0; j < methods.size(); j++)
            if (methods[j]->getName() == name) {
                method = methods[j];
		flag = true;
            }
	if(flag)
	  {
	    break;
	  }
    }

    if (method == 0) {
        error = true;
       classtable->semant_error(this) << "Static dispatch to undefined method " << name << ".\n";
    } else {
        Formals formals = method->getFormals();
        int it1 = actual->first(), it2 = formals->first();
        while (actual->more(it1) && formals->more(it2)) {

            Symbol actual_type = actual->nth(it1)->checkType();
            Symbol formal_type = formals->nth(it2)->getType();

            if (!(classtable->is_conform_to(actual_type, formal_type))) {
                error = true;
                classtable->semant_error(this) << "In call of method " << name << ", type " << actual_type << " of parameter " << formals->nth(it2)->getName() << " does not conform to declared type " << formal_type << ".\n";
            }
            it1 = actual->next(it1);
            it2 = formals->next(it2);
            if (actual->more(it1) xor formals->more(it2)) {
                error = true;
                classtable->semant_error(this) << "Method " << name << " called with wrong number of arguments.\n";
            }
        }
    }

    if (error) {
        type = Object;
    } else {
        type = method->getReturnType();
        if (type == SELF_TYPE)
            type = this->type_name;
    }

    return type;
}


/////////////////////////////////////////////////
//Check that if the method name exist in features of the class or its ancestor or not
//Check about the formals and actuals(their Type and their numbers)
//////////////////////////////////////////////

Symbol dispatch_class::checkType() {
    bool error = false;
    Symbol expr_type = expr->checkType();

    if (expr_type != SELF_TYPE && classTable.find(expr_type) == classTable.end()) {
        classtable->semant_error(this) << "Dispatch on undefined class " << expr_type << ".\n";
        type = Object;
        return type;
    }
    bool flag = false;
    method_class* method = 0;
    std::vector<Class_> class_inh_path = classtable->getInheritancePath(expr_type);
    for (size_t i = 0; i < class_inh_path.size(); i++) {
        Methods methods = methodTable[class_inh_path[i]];
        for (size_t j = 0; j < methods.size(); j++)
            if (methods[j]->getName() == name) {
                method = methods[j];
                flag = true; 
            }
	if(flag)
	  {
	    break;
	  }
    }

    if (method == 0) {
        error = true;
        classtable->semant_error(this) << "Dispatch to undefined method " << name << ".\n";
    } else {
        Formals formals = method->getFormals();
        int it1 = actual->first(), it2 = formals->first();
        while (actual->more(it1) && formals->more(it2)) {
            
	    Symbol actual_type = actual->nth(it1)->checkType();
            Symbol formal_type = formals->nth(it2)->getType();
            
	    if (not (classtable->is_conform_to(actual_type, formal_type))) {
                error = true;
                classtable->semant_error(this) << "In call of method " << name << ", type " << actual_type << " of parameter " << formals->nth(it2)->getName() << " does not conform to declared type " << formal_type << ".\n";
            }
            it1 = actual->next(it1);
            it2 = formals->next(it2);
            if (actual->more(it1) xor formals->more(it2)) {
                error = true;
                classtable->semant_error(this) << "Method " << name << " called with wrong number of arguments.\n";
            }
        }
    }

    if (error) {
        type = Object;
    } else {
        type = method->getReturnType();
        if (type == SELF_TYPE)
            type = expr_type;
    }

    return type;
}

////////////////////////////////////
// for type of conditional we should consider the lowest common ancestor
////////////////////////////////////

Symbol cond_class::checkType() {
    if (pred->checkType() != Bool)
        classtable->semant_error(this) << "Predicate of 'if' does not have type Bool.\n";

    Symbol then_type = then_exp->checkType();
    Symbol else_type = else_exp->checkType();

    if (then_type == SELF_TYPE && else_type == SELF_TYPE)
        type = SELF_TYPE;
    else
      type = (classtable->LCA(then_type, else_type))->getName();
    return type;
}

/////////////////////////////
// loop type can be any registered type so we can consider Object for the type of loop class
////////////////////////////

Symbol loop_class::checkType() {
    if (pred->checkType() != Bool)
        classtable->semant_error(this) << "Loop condition does not have type Bool.\n";
    body->checkType();
    type = Object;
    return type;
}
//////////////////////////
// for case class the type is the LCA of all branch types
/////////////////////////

Symbol typcase_class::checkType() {
    Symbol expr_type = expr->checkType();

    std::set<Symbol> branch_type_decls;
    
    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
        branch_class* branch = static_cast<branch_class*>(cases->nth(i));

        if (branch_type_decls.find(branch->get_type_decl()) != branch_type_decls.end())

            classtable->semant_error(branch) << "Duplicate branch " << branch->get_type_decl() << " in case statement.\n";
        else
            branch_type_decls.insert(branch->get_type_decl());

        Symbol branch_type = branch->checkType();
        if (i == cases->first())
            type = branch_type;

        else if (type != SELF_TYPE || branch_type != SELF_TYPE)
	  type = (classtable->LCA(type, branch_type))->getName();
    }

    return type;
}

/////////////////////////
//branch type is equal to its expr type
////////////////////////
Symbol branch_class::checkType() {
    objectEnv.enterscope();
    objectEnv.addid(name, new Symbol(type_decl));
    type = expr->checkType();
    objectEnv.exitscope();
    return type;
}

//////////////////////////
// for block type we should return the last expr type
/////////////////////////

Symbol block_class::checkType() {
    for (int i = body->first(); body->more(i); i = body->next(i))
        type = body->nth(i)->checkType();
    return type;
}

/////////////////////////
//let class type is equal to its body type
////////////////////////

Symbol let_class::checkType() {
    objectEnv.enterscope();
    objectEnv.addid(identifier, new Symbol(type_decl));

    Symbol init_type = init->checkType();

    if (classTable.find(type_decl) == classTable.end())
        classtable->semant_error(this) << "Class " << type_decl << " of let-bound identifier " << identifier << " is undefined.\n";

    else if (init_type != No_type && !classtable->is_conform_to(init_type, type_decl))
        classtable->semant_error(this) << "Inferred type " << init_type << " of initialization of " << identifier << " does not conform to identifier's declared type " << type_decl << ".\n";

    type = body->checkType();
    objectEnv.exitscope();
    return type;
}

Symbol plus_class::checkType() {
    Symbol type1 = e1->checkType();
    Symbol type2 = e2->checkType();
    if (type1 != Int || type2 != Int)
        classtable->semant_error(this) << "non-Int arguments: " << type1 << " + " << type2 << endl;
    type = Int;
    return type;
}

Symbol sub_class::checkType() {
    Symbol type1 = e1->checkType();
    Symbol type2 = e2->checkType();
    if (type1 != Int || type2 != Int)
        classtable->semant_error(this) << "non-Int arguments: " << type1 << " - " << type2 << endl;
    type = Int;
    return type;
}

Symbol mul_class::checkType() {
    Symbol type1 = e1->checkType();
    Symbol type2 = e2->checkType();
    if (type1 != Int || type2 != Int)
       classtable->semant_error(this) << "non-Int arguments: " << type1 << " * " << type2 << endl;
    type = Int;
    return type;
}

Symbol divide_class::checkType() {
    Symbol type1 = e1->checkType();
    Symbol type2 = e2->checkType();
    if (type1 != Int || type2 != Int)
        classtable->semant_error(this) << "non-Int arguments: " << type1 << " / " << type2 << endl;
    type = Int;
    return type;
}

Symbol neg_class::checkType() {
    type = e1->checkType();
    if (type != Int)
        classtable->semant_error(this) << "Argument of '~' has type " << type << " instead of Int.\n";
    type = Int;
    return type;
}

Symbol lt_class::checkType() {
    Symbol type1 = e1->checkType();
    Symbol type2 = e2->checkType();
    if (type1 != Int || type2 != Int)
        classtable->semant_error(this) << "non-Int arguments: " << type1 << " < " << type2 << endl;
    type = Bool;
    return type;
}

Symbol eq_class::checkType() {
    Symbol t1 = e1->checkType(), t2 = e2->checkType();
    if ((t1 == Int || t1 == Bool || t1 == Str || t2 == Int || t2 == Bool || t2 == Str) && t1 != t2)
        classtable->semant_error(this) << "Illegal comparison with a basic type.\n";
    type = Bool;
    return type;
}

Symbol leq_class::checkType() {
    Symbol type1 = e1->checkType();
    Symbol type2 = e2->checkType();
    if (type1 != Int || type2 != Int)
        classtable->semant_error(this) << "non-Int arguments: " << type1 << " <= " << type2 << endl;
    type = Bool;
    return type;
}

Symbol comp_class::checkType() {
    type = e1->checkType();
    if (type != Bool)
      classtable->semant_error(this) << "Argument of 'not' has type " << type << " instead of Bool.\n";
    type = Bool;
    return type;
}

Symbol int_const_class::checkType() {
    type = Int;
    return type;
}

Symbol bool_const_class::checkType() {
    type = Bool;
    return type;
}

Symbol string_const_class::checkType() {
    type = Str;
    return type;
}

Symbol new__class::checkType() {
    if (this->type_name != SELF_TYPE && classTable.find(this->type_name) == classTable.end()) {
        classtable->semant_error(this) << "'new' used with undefined class " << this->type_name << ".\n";
        this->type_name = Object;
    }
    type = this->type_name;
    return type;
}

Symbol isvoid_class::checkType() {
    e1->checkType();
    type = Bool;
    return type;
}

Symbol no_expr_class::checkType() {
    type = No_type;
    return type;
}

Symbol object_class::checkType() {
    if (name == self) {
        type = SELF_TYPE;
    } else if (objectEnv.lookup(name)) {
        type = *objectEnv.lookup(name);
    } else {
        classtable->semant_error(this) << "Undeclared identifier " << name << ".\n";
        type = Object;
    }
    return type;
}

void program_class::semant() {
    initialize_constants();
    classtable = new ClassTable(classes);
    classtable->register_basic_classes();
    classtable->register_classes(classes);
    classtable->check_inheritance();
    
    if (classtable->semant_errors > 0) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
    
    classtable->check_main();
    classtable->register_methods();
    classtable->check_methods();

    if (classtable->semant_errors > 0) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}

