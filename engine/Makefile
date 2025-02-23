CXX = g++
AR = ar
LD = g++
WINDRES = windres

INC = -I../dependencies/glm/include -I../dependencies/freeglut/include
CFLAGS = -Wall -std=c++20 -fPIC -m64 -fexceptions
RCFLAGS =
RESINC =
LIBDIR = -L../dependencies/freeglut/lib
LIB = -lGL -lGLU -lglut -lfreeimage
LDFLAGS = -m64

INC_DEBUG = $(INC)
CFLAGS_DEBUG = $(CFLAGS) -g -D_DEBUG
RESINC_DEBUG = $(RESINC)
RCFLAGS_DEBUG = $(RCFLAGS)
LIBDIR_DEBUG = $(LIBDIR)
LIB_DEBUG = $(LIB)
LDFLAGS_DEBUG = $(LDFLAGS)
OBJDIR_DEBUG = obj/Debug
DEP_DEBUG =
OUT_DEBUG = bin/Debug/libengine.so

INC_RELEASE = $(INC)
CFLAGS_RELEASE = $(CFLAGS) -O2
RESINC_RELEASE = $(RESINC)
RCFLAGS_RELEASE = $(RCFLAGS)
LIBDIR_RELEASE = $(LIBDIR)
LIB_RELEASE = $(LIB)
LDFLAGS_RELEASE = $(LDFLAGS) -s
OBJDIR_RELEASE = obj/Release
DEP_RELEASE =
OUT_RELEASE = bin/Release/libengine.so

# Lista di tutti i file sorgente
SRCS = engine.cpp \
       Camera.cpp \
       DirectionalLight.cpp \
       Light.cpp \
       Material.cpp \
       Mesh.cpp \
       Node.cpp \
       Object.cpp \
       OrthographicCamera.cpp \
       PerspectiveCamera.cpp \
       PointLight.cpp \
       SpotLight.cpp \
       Texture.cpp \
       List.cpp \
       ListElement.cpp \
       Vertex.cpp \
       OvoReader.cpp \
       CallbackManager.cpp

TEST_SRCS = Tests/Test_Main.cpp \
            Tests/Test_Camera.cpp \
            Tests/Test_Light.cpp \
            Tests/Test_Node.cpp \
            Tests/Test_List.cpp \
            Tests/Test_Mesh.cpp \
            Tests/Test_CallManager.cpp

# Genera la lista degli oggetti per Debug e Release
OBJ_DEBUG = $(SRCS:%.cpp=$(OBJDIR_DEBUG)/%.o)
OBJ_RELEASE = $(SRCS:%.cpp=$(OBJDIR_RELEASE)/%.o)

TEST_OBJ_DEBUG = $(TEST_SRCS:%.cpp=$(OBJDIR_DEBUG)/%.o)
TEST_OBJ_RELEASE = $(TEST_SRCS:%.cpp=$(OBJDIR_RELEASE)/%.o)

all: debug release test_debug test_release

clean: clean_debug clean_release clean_test_debug clean_test_release

before_debug:
	test -d bin/Debug || mkdir -p bin/Debug
	test -d $(OBJDIR_DEBUG) || mkdir -p $(OBJDIR_DEBUG)
	test -d $(OBJDIR_DEBUG)/Tests || mkdir -p $(OBJDIR_DEBUG)/Tests

after_debug:

debug: before_debug out_debug after_debug

out_debug: before_debug $(OBJ_DEBUG) $(DEP_DEBUG)
	$(LD) -shared $(LIBDIR_DEBUG) $(OBJ_DEBUG) -o $(OUT_DEBUG) $(LDFLAGS_DEBUG) $(LIB_DEBUG)

# Regola per compilare i file sorgente in Debug
$(OBJDIR_DEBUG)/%.o: %.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c $< -o $@

clean_debug:
	rm -f $(OBJ_DEBUG) $(OUT_DEBUG)
	rm -rf bin/Debug
	rm -rf $(OBJDIR_DEBUG)

before_release:
	test -d bin/Release || mkdir -p bin/Release
	test -d $(OBJDIR_RELEASE) || mkdir -p $(OBJDIR_RELEASE)
	test -d $(OBJDIR_RELEASE)/Tests || mkdir -p $(OBJDIR_RELEASE)/Tests

after_release:

release: before_release out_release after_release

out_release: before_release $(OBJ_RELEASE) $(DEP_RELEASE)
	$(LD) -shared $(LIBDIR_RELEASE) $(OBJ_RELEASE) -o $(OUT_RELEASE) $(LDFLAGS_RELEASE) $(LIB_RELEASE)

# Regola per compilare i file sorgente in Release
$(OBJDIR_RELEASE)/%.o: %.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c $< -o $@

clean_release:
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)
	rm -rf bin/Release
	rm -rf $(OBJDIR_RELEASE)


test_debug: before_debug out_debug $(TEST_OBJ_DEBUG)
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) $(OBJ_DEBUG) $(TEST_OBJ_DEBUG) -o bin/Debug/tests $(LIB_DEBUG) $(LDFLAGS_DEBUG)
	@echo "Running Debug Tests..."
	@LD_LIBRARY_PATH=./bin/Debug:../dependencies/freeglut/lib \
	./bin/Debug/tests 2>&1 | tee debug_test_output.txt; \
	TEST_EXIT_CODE=$$?; \
	if [ $$TEST_EXIT_CODE -ne 0 ]; then \
		echo "Test output:" && cat debug_test_output.txt && exit 1; \
	fi

test_release: before_release out_release $(TEST_OBJ_RELEASE)
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) $(OBJ_RELEASE) $(TEST_OBJ_RELEASE) -o bin/Release/tests $(LIB_RELEASE) $(LDFLAGS_RELEASE)
	@echo "Running Release Tests..."
	@LD_LIBRARY_PATH=./bin/Release:../dependencies/freeglut/lib \
	./bin/Release/tests 2>&1 | tee release_test_output.txt; \
	TEST_EXIT_CODE=$$?; \
	if [ $$TEST_EXIT_CODE -ne 0 ]; then \
		echo "Test output:" && cat release_test_output.txt && exit 1; \
	fi

test: test_debug test_release
	@echo "All Tests Completed!"

clean_test_debug:
	rm -f $(TEST_OBJ_DEBUG) bin/Debug/tests

clean_test_release:
	rm -f $(TEST_OBJ_RELEASE) bin/Release/tests

.PHONY: before_debug after_debug clean_debug before_release after_release clean_release