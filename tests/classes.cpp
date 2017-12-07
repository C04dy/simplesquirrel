#define CATCH_CONFIG_MAIN 
#include <catch/catch.hpp>
#include "../include/squirrelbind/squirrelbind.hpp"

#define STRINGIFY(x) #x

using namespace SquirrelBind;

SQInteger print_args(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v); //number of arguments
    for(SQInteger n=1;n<=nargs;n++)
    {
        printf("arg %d is ",n);
        switch(sq_gettype(v,n))
        {
            case OT_NULL:
                printf("null");
                break;
            case OT_INTEGER:
                printf("integer");
                break;
            case OT_FLOAT:
                printf("float");
                break;
            case OT_STRING:
                printf("string");
                break;
            case OT_TABLE:
                printf("table");
                break;
            case OT_ARRAY:
                printf("array");
                break;
            case OT_USERDATA:
                printf("userdata");
                break;
            case OT_CLOSURE:
                printf("closure(function)");
                break;
            case OT_NATIVECLOSURE:
                printf("native closure(C function)");
                break;
            case OT_GENERATOR:
                printf("generator");
                break;
            case OT_USERPOINTER:
                printf("userpointer");
                break;
            case OT_CLASS:
                printf("class");
                break;
            case OT_INSTANCE:
                printf("instance");
                break;
            case OT_WEAKREF:
                printf("weak reference");
                break;
            default:
                return sq_throwerror(v,"invalid param"); //throws an exception
        }
    }
    printf("\n");
    sq_pushinteger(v,nargs); //push the number of arguments as return value
    return 1; //1 because 1 value is returned
}

TEST_CASE("Find class"){
    static const std::string source = STRINGIFY(
        class Vector {
            constructor(x, y, z) {
                this.x = x;
                this.y = y;
                this.z = z;
            }

            function getX() {
                return x;
            }

            function getY() {
                return y;
            }

            function getZ() {
                return z;
            }
            
            x = 0;
            y = 0;
            z = 0;
        };
    );

    SqVM vm(1024, SqLibs::ALL);
    SqScript script = vm.compileSource(source.c_str());
    vm.run(script);

    SqClass vectorClass = vm.findClass("Vector");
    SqInstance vector = vm.newInstance(vectorClass, 5, 10, 15);

    REQUIRE(vector.isEmpty() == false);

    SqFunction getX = vectorClass.findFunc("getX");
    SqFunction getY = vectorClass.findFunc("getY");
    SqFunction getZ = vectorClass.findFunc("getZ");

    REQUIRE(vm.callFunc(getX, vector).toInt() == 5);
    REQUIRE(vm.callFunc(getY, vector).toInt() == 10);
    REQUIRE(vm.callFunc(getZ, vector).toInt() == 15);
}

TEST_CASE("Register class") {
	class Foo;

	static Foo* ptr;

    class Foo: public SqClassWrapper<Foo> {
    public:
        Foo(int val):val(val) {
			ptr = this;
        }

		void setVal(int val) {
			this->val = val;
        }

        int getVal() const {
            return val;
        }

		void doStuff() {
			this->val = -100;
        }

        static SqClass expose(SqVM& vm){
            SqClass cls = vm.addClass("Foo", SqClass::Ctor<Foo(int)>());

			cls.addFunc("setVal", &Foo::setVal);
			cls.addFunc("doStuff", &Foo::doStuff);
			cls.addFunc("getVal", &Foo::getVal);

            return cls;
        }

        int val;
    };
    
    static const std::string source = STRINGIFY(
        local instance = null;
        
        function bar() {
            instance = Foo(42);
			return typeof Foo;
        }

		function baz() {
			return typeof Foo.setVal;
        }

		function baz2() {
			instance.setVal(123456);
        }

		function baz3() {
			instance.doStuff();
        }

		function baz4() {
			return instance.getVal();
        }
    );

    SqVM vm(1024, SqLibs::ALL);
    SqScript script = vm.compileSource(source.c_str());
    vm.run(script);

    vm.expose<Foo>();

	SqFunction bar = vm.findFunc("bar");
	std::string type = vm.callFunc(bar, vm).toString();

	REQUIRE(type == "class");

	SqFunction baz = vm.findFunc("baz");
	type = vm.callFunc(baz, vm).toString();

	REQUIRE(type == "function");

	SqFunction baz2 = vm.findFunc("baz2");
	vm.callFunc(baz2, vm);

	REQUIRE(ptr->getVal() == 123456);

	SqFunction baz3 = vm.findFunc("baz3");
	vm.callFunc(baz3, vm);

	REQUIRE(ptr->getVal() == -100);

	ptr->setVal(102030);

	SqFunction baz4 = vm.findFunc("baz4");
	auto ret = vm.callFunc(baz4, vm).toInt();

	REQUIRE(ret == 102030);
}

TEST_CASE("Register class with std::string type") {
	class Foo;

	static Foo* ptr;

	class Foo : public SqClassWrapper<Foo> {
	public:
		Foo(const std::string& val) :val(val) {
			ptr = this;
		}

		void setVal(const std::string& val) {
			this->val = val;
		}

		const std::string& getVal() const {
			return val;
		}

		void doStuff() {
			this->val = "Potato";
		}

		static SqClass expose(SqVM& vm) {
			SqClass cls = vm.addClass("Foo", SqClass::Ctor<Foo(std::string)>());

			cls.addFunc("setVal", &Foo::setVal);
			cls.addFunc("doStuff", &Foo::doStuff);
			cls.addFunc("getVal", &Foo::getVal);

			return cls;
		}

		std::string val;
	};

	static const std::string source = STRINGIFY(
        local instance = null;
        
        function bar() {
            instance = Foo("Hello World");
			return typeof Foo;
        }

		function baz() {
			return typeof Foo.setVal;
        }

		function baz2() {
			instance.setVal("Banana");
        }

		function baz3() {
			instance.doStuff();
        }

		function baz4() {
			return instance.getVal();
        }
    );

    SqVM vm(1024, SqLibs::ALL);
    SqScript script = vm.compileSource(source.c_str());
    vm.run(script);

    vm.expose<Foo>();

	SqFunction bar = vm.findFunc("bar");
	std::string type = vm.callFunc(bar, vm).toString();

	REQUIRE(type == "class");

	SqFunction baz = vm.findFunc("baz");
	type = vm.callFunc(baz, vm).toString();

	REQUIRE(type == "function");

	SqFunction baz2 = vm.findFunc("baz2");
	vm.callFunc(baz2, vm);

	REQUIRE(ptr->getVal() == "Banana");

	SqFunction baz3 = vm.findFunc("baz3");
	vm.callFunc(baz3, vm);

	REQUIRE(ptr->getVal() == "Potato");

	ptr->setVal("Squirrel");

	SqFunction baz4 = vm.findFunc("baz4");
	auto ret = vm.callFunc(baz4, vm).toString();

	REQUIRE(ret == "Squirrel");
}

template<typename T>
static void testTypeClass(T value) {
	class Foo;

	static Foo* ptr;
	
	class Foo: public SqClassWrapper<Foo> {
	public:
		Foo(const T& value):value(value) {
			ptr = this;
		}

		~Foo() {
			
		}

		const T& getValueRefConst() const {
			return value;
		}

		T& getValueRef() {
			return value;
		}

		T getValue() {
			return value;
		}

		void setValueRefConst(const T& value) {
			this->value = value;
		}

		void setValue(T value) {
			this->value = value;
		}

		static SqClass expose(SqVM& vm) {
			SqClass cls = vm.addClass("Foo", SqClass::Ctor<Foo(T)>());

			cls.addFunc("getValueRefConst", &Foo::getValueRefConst);
			cls.addFunc("getValueRef", &Foo::getValueRef);
			cls.addFunc("getValue", &Foo::getValue);

			cls.addFunc("setValueRefConst", &Foo::setValueRefConst);
			cls.addFunc("setValue", &Foo::setValue);

			return cls;
		}

		T value;
	};
	
	static const std::string source = STRINGIFY(
		local foo = Foo(42);
		
		function get() {
			return foo.getValue();
		}

		function getByRef() {
			return foo.getValueRef();
		}

		function getByRefConst() {
			return foo.getValueRefConst();
		}

		function set(value) {
			foo.setValue(value);
		}

		function setByRefConst(value) {
			foo.setValueRefConst(value);
		}
    );

	SqVM vm(1024);
	SqScript script = vm.compileSource(source.c_str());

	vm.expose<Foo>();
    vm.run(script);

	SqFunction get = vm.findFunc("get");
	SqFunction getByRef = vm.findFunc("getByRef");
	SqFunction getByRefConst = vm.findFunc("getByRefConst");
	SqFunction set = vm.findFunc("set");
	SqFunction setByRefConst = vm.findFunc("setByRefConst");

	T getResult = vm.callFunc(get, vm).to<T>();
	REQUIRE(getResult == 42);

	T getByRefResult = vm.callFunc(getByRef, vm).to<T>();
	REQUIRE(getByRefResult == 42);

	T getByRefConstResult = vm.callFunc(getByRefConst, vm).to<T>();
	REQUIRE(getByRefConstResult == 42);

	vm.callFunc(set, vm, value);

	REQUIRE(ptr->value == value);

	ptr->value = 0;

	vm.callFunc(setByRefConst, vm, value);

	REQUIRE(ptr->value == value);
}

template<>
static void testTypeClass(std::string value) {
	class Foo;

	static Foo* ptr;
	
	class Foo: public SqClassWrapper<Foo> {
	public:
		Foo(const std::string& value):value(value) {
			ptr = this;
		}

		~Foo() {
			
		}

		const std::string& getValueRefConst() const {
			return value;
		}

		std::string& getValueRef() {
			return value;
		}

		std::string getValue() {
			return value;
		}

		void setValueRefConst(const std::string& value) {
			this->value = value;
		}

		void setValue(std::string value) {
			this->value = value;
		}

		static SqClass expose(SqVM& vm) {
			SqClass cls = vm.addClass("Foo", SqClass::Ctor<Foo(std::string)>());

			cls.addFunc("getValueRefConst", &Foo::getValueRefConst);
			cls.addFunc("getValueRef", &Foo::getValueRef);
			cls.addFunc("getValue", &Foo::getValue);

			cls.addFunc("setValueRefConst", &Foo::setValueRefConst);
			cls.addFunc("setValue", &Foo::setValue);

			return cls;
		}

		std::string value;
	};
	
	static const std::string source = STRINGIFY(
		local foo = Foo("Heyo");
		
		function get() {
			return foo.getValue();
		}

		function getByRef() {
			return foo.getValueRef();
		}

		function getByRefConst() {
			return foo.getValueRefConst();
		}

		function set(value) {
			foo.setValue(value);
		}

		function setByRefConst(value) {
			foo.setValueRefConst(value);
		}
    );

	SqVM vm(1024);
	SqScript script = vm.compileSource(source.c_str());

	vm.expose<Foo>();
    vm.run(script);

	SqFunction get = vm.findFunc("get");
	SqFunction getByRef = vm.findFunc("getByRef");
	SqFunction getByRefConst = vm.findFunc("getByRefConst");
	SqFunction set = vm.findFunc("set");
	SqFunction setByRefConst = vm.findFunc("setByRefConst");

	std::string getResult = vm.callFunc(get, vm).to<std::string>();
	REQUIRE(getResult == "Heyo");

	std::string getByRefResult = vm.callFunc(getByRef, vm).to<std::string>();
	REQUIRE(getByRefResult == "Heyo");

	std::string getByRefConstResult = vm.callFunc(getByRefConst, vm).to<std::string>();
	REQUIRE(getByRefConstResult == "Heyo");

	vm.callFunc(set, vm, value);

	REQUIRE(ptr->value == value);

	ptr->value = "";

	vm.callFunc(setByRefConst, vm, value);

	REQUIRE(ptr->value == value);
}

TEST_CASE("Test passing signed char member function") {
	testTypeClass<signed char>(30);
}

TEST_CASE("Test passing char member function") {
	testTypeClass<char>(66);
}

TEST_CASE("Test passing short member function") {
	testTypeClass<short>(25365);
}

TEST_CASE("Test passing int member function") {
	testTypeClass<int>(30504030);
}

TEST_CASE("Test passing long member function") {
	testTypeClass<long>(30504030);
}

TEST_CASE("Test passing unsigned char member function") {
	testTypeClass<unsigned char>(0xFA);
}

TEST_CASE("Test passing unsigned short member function") {
	testTypeClass<unsigned short>(0xFA85);
}

TEST_CASE("Test passing unsigned int member function") {
	testTypeClass<unsigned int>(0xFF504030);
}

TEST_CASE("Test passing unsigned long member function") {
	testTypeClass<unsigned long>(0xFF504030);
}

#ifdef SQUSEDOUBLE
TEST_CASE("Test passing double") {
	testTypeClass<double>(0.122554578.0, "float");
}
#endif

#ifdef _SQ64
TEST_CASE("Test passing unsigned long long") {
	testTypeClass<unsigned long long>(0xFFCCBBAAFF429587);
}

TEST_CASE("Test passing long long") {
	testTypeClass<long long>(0x12746585FF429587);
}

TEST_CASE("Test passing uint64_t") {
	testTypeClass<uint64_t>(0xFFCCBBAAFF429587);
}

TEST_CASE("Test passing int64_t") {
	testTypeClass<int64_t>(0x12746585FF429587);
}
#endif

#ifdef SQUNICODE
TEST_CASE("Test passing std::wstring") {
	testTypeClass<std::wstring>(L"Hello World!");
}
#else
TEST_CASE("Test passing std::string") {
	testTypeClass<std::string>("Hello World!");
}
#endif