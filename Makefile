MAKE = make

all: build_engine build_client package

build_engine:
	$(MAKE) -C engine all

build_client: build_engine
	$(MAKE) -C client all

clean: clean_engine clean_client
	rm -rf dist test_extract graphics_engine_*.tar.gz

clean_engine:
	$(MAKE) -C engine clean

clean_client:
	$(MAKE) -C client clean

package: build_engine build_client
	@echo "Creating distribution package..."
	@mkdir -p dist
	@cp engine/bin/Release/libengine.so dist/
	@cp client/bin/Release/* dist/
	@TIMESTAMP=$$(date +%Y%m%d_%H%M%S); \
	tar -czf graphics_engine_$$TIMESTAMP.tar.gz dist/
	@rm -rf dist
	@echo "Package created successfully"

test_package: package
	@echo "Testing package contents..."
	@mkdir -p test_extract
	@PACKAGE_FILE=$$(ls -t graphics_engine_*.tar.gz | head -n1); \
	if [ -z "$$PACKAGE_FILE" ]; then \
		echo "ERROR: No package file found!"; \
		exit 1; \
	fi; \
	echo "Testing package: $$PACKAGE_FILE"; \
	echo "Package contents:"; \
	tar -tvf "$$PACKAGE_FILE"; \
	echo ""; \
	if ! tar -xzf "$$PACKAGE_FILE" -C test_extract; then \
		echo "ERROR: Failed to extract package!"; \
		rm -rf test_extract; \
		exit 1; \
	fi; \
	errors=0; \
	for file in test_extract/dist/libengine.so test_extract/dist/client; do \
		if [ ! -f "$$file" ]; then \
			echo "ERROR: Required file $$file is missing!"; \
			errors=1; \
		else \
			echo "Found $$file"; \
			if [ -x "$$file" ]; then \
				echo "$$file has execution permissions"; \
			fi; \
		fi; \
	done; \
	if [ ! -x test_extract/dist/client ]; then \
		echo "ERROR: Client does not have execution permissions!"; \
		errors=1; \
	fi; \
	if [ $$errors -eq 0 ]; then \
		echo "All package tests passed successfully!"; \
	else \
		echo "Package tests failed!"; \
		rm -rf test_extract; \
		exit 1; \
	fi; \
	rm -rf test_extract

.PHONY: clean_engine clean_client package test_package all build_engine build_client