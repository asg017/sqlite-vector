ifeq ($(shell uname -s),Darwin)
CONFIG_DARWIN=y
else ifeq ($(OS),Windows_NT)
CONFIG_WINDOWS=y
else
CONFIG_LINUX=y
endif

LIBRARY_PREFIX=lib
ifdef CONFIG_DARWIN
LOADABLE_EXTENSION=dylib
endif

ifdef CONFIG_LINUX
LOADABLE_EXTENSION=so
endif


ifdef CONFIG_WINDOWS
LOADABLE_EXTENSION=dll
LIBRARY_PREFIX=
endif

ifdef python
PYTHON=$(python)
else
PYTHON=python3
endif

prefix=dist

TARGET_LOADABLE=$(prefix)/debug/vector0.$(LOADABLE_EXTENSION)
TARGET_LOADABLE_RELEASE=$(prefix)/release/vector0.$(LOADABLE_EXTENSION)

TARGET_WHEELS=$(prefix)/debug/wheels
TARGET_WHEELS_RELEASE=$(prefix)/release/wheels

INTERMEDIATE_PYPACKAGE_EXTENSION=python/sqlite_vector/sqlite_vector/vector0.$(LOADABLE_EXTENSION)

$(prefix):
	mkdir -p $(prefix)/debug
	mkdir -p $(prefix)/release

$(TARGET_LOADABLE): $(prefix) src/extension.cpp
	cmake -B build; make -C build
	cp build/vector0.$(LOADABLE_EXTENSION) $@

$(TARGET_LOADABLE_RELEASE): $(prefix)
	cmake -DCMAKE_BUILD_TYPE=Release -B build_release; make -C build_release
	cp build_release/vector0.$(LOADABLE_EXTENSION) $@

$(TARGET_WHEELS): $(prefix)
	mkdir -p $(TARGET_WHEELS)

$(TARGET_WHEELS_RELEASE): $(prefix)
	mkdir -p $(TARGET_WHEELS_RELEASE)


loadable: $(TARGET_LOADABLE)

loadable-release: $(TARGET_LOADABLE_RELEASE)


python: $(TARGET_WHEELS) $(TARGET_LOADABLE) python/sqlite_vector/setup.py python/sqlite_vector/sqlite_vector/__init__.py .github/workflows/rename-wheels.py
	cp $(TARGET_LOADABLE) $(INTERMEDIATE_PYPACKAGE_EXTENSION) 
	rm $(TARGET_WHEELS)/sqlite_vector* || true
	pip3 wheel python/sqlite_vector/ -w $(TARGET_WHEELS)
	python3 .github/workflows/rename-wheels.py $(TARGET_WHEELS) $(RENAME_WHEELS_ARGS)

python-release: $(TARGET_LOADABLE_RELEASE) $(TARGET_WHEELS_RELEASE) python/sqlite_vector/setup.py python/sqlite_vector/sqlite_vector/__init__.py .github/workflows/rename-wheels.py
	cp $(TARGET_LOADABLE_RELEASE)  $(INTERMEDIATE_PYPACKAGE_EXTENSION) 
	rm $(TARGET_WHEELS_RELEASE)/sqlite_vector* || true
	pip3 wheel python/sqlite_vector/ -w $(TARGET_WHEELS_RELEASE)
	python3 .github/workflows/rename-wheels.py $(TARGET_WHEELS_RELEASE) $(RENAME_WHEELS_ARGS)

datasette: $(TARGET_WHEELS) python/datasette_sqlite_vector/setup.py python/datasette_sqlite_vector/datasette_sqlite_vector/__init__.py
	rm $(TARGET_WHEELS)/datasette* || true
	pip3 wheel python/datasette_sqlite_vector/ --no-deps -w $(TARGET_WHEELS)

datasette-release: $(TARGET_WHEELS_RELEASE) python/datasette_sqlite_vector/setup.py python/datasette_sqlite_vector/datasette_sqlite_vector/__init__.py
	rm $(TARGET_WHEELS_RELEASE)/datasette* || true
	pip3 wheel python/datasette_sqlite_vector/ --no-deps -w $(TARGET_WHEELS_RELEASE)


test-loadable:
	$(PYTHON) tests/test-loadable.py

test-python:
	$(PYTHON) tests/test-python.py

test:
	make test-loadable
	make test-python

.PHONY: clean \
	test test-loadable test-python \
	loadable loadable-release \
	python python-release \
	datasette datasette-release \
	static static-release \
	debug release