LIBNAME = tileop-api
VERSION = 0.58.3
HEADERS = $(wildcard include/*.h) $(wildcard include/*.hpp) include/jcore include/cpu_sim include/aarch64 include/common


# install to system include directory of Clang
CLANG_PREFIX ?=
INSTALL_DIR = $(shell $(CLANG_PREFIX)/bin/clang -print-resource-dir)/include/$(LIBNAME)

.PHONY: check install uninstall

check:
	python3 tools/generate_engine_docs.py --check
	python3 tools/check_tileop_usage_examples.py
	python3 test/test_v058_engine_contract.py
	$(CXX) -std=c++20 -D__linx -include test/linx_host_type_shim.hpp \
		-fsyntax-only -Iinclude test/ptoas_linx_type_compat.cpp
	$(CXX) -std=c++20 -D__linx -include test/linx_host_type_shim.hpp \
		-fsyntax-only -Iinclude test/pto0583_contract.cpp
	bash -n test/tileop_api/compile.all test/tileop_api/run_negatives.sh \
		test/tileop_api/verify_pto0583_asm.sh \
		test/tileop_api/verify_target_cxx_frontend.sh
	@if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then \
		git diff --check; \
	fi

install:
	@echo "Installing $(LIBNAME) to Clang toolchain at $(INSTALL_DIR)"
	@mkdir -p $(INSTALL_DIR)
	@cp -r $(HEADERS) $(INSTALL_DIR)
	@echo "Installation complete. Now you can use #include <$(LIBNAME)/header.h>"

uninstall:
	@echo "Removing $(INSTALL_DIR)"
	@rm -rf $(INSTALL_DIR)
	@echo "Uninstall complete"
