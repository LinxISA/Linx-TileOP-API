LIBNAME = tileop-api
VERSION = 0.58.3
HEADERS = $(wildcard include/*.h) $(wildcard include/*.hpp) include/jcore include/cpu_sim include/aarch64 include/common
TILEOP_API_REVISION ?= $(shell git rev-parse HEAD 2>/dev/null || printf unknown)
TILEOP_API_REVISION_IS_EXACT ?= $(shell test -z "$$(git status --porcelain --untracked-files=all 2>/dev/null)" && printf 1 || printf 0)


# install to system include directory of Clang
CLANG_PREFIX ?=
INSTALL_DIR = $(shell $(CLANG_PREFIX)/bin/clang -print-resource-dir)/include/$(LIBNAME)

.PHONY: check install uninstall

check:
	python3 tools/generate_engine_docs.py --check
	python3 tools/check_issue_49_docs.py
	python3 tools/check_tileop_usage_examples.py
	python3 tools/check_no_legacy_tileop_api.py
	python3 test/test_v058_engine_contract.py
	python3 test/test_pto0585_layout_interfaces.py
	python3 test/test_version_header.py
	$(CXX) -std=c++20 -D__linx -include test/linx_host_type_shim.hpp \
		-fsyntax-only -Iinclude test/ptoas_linx_type_compat.cpp
	$(CXX) -std=c++20 -D__linx -include test/linx_host_type_shim.hpp \
		-fsyntax-only -Iinclude test/pto0583_contract.cpp
	bash -n test/tileop_api/compile.all test/tileop_api/run_negatives.sh \
		test/tileop_api/verify_pto0583_asm.sh \
		test/tileop_api/verify_shared_last_use.sh \
		test/tileop_api/verify_target_cxx_frontend.sh
	@if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then \
		git diff --check; \
	fi

install:
	@echo "Installing $(LIBNAME) to Clang toolchain at $(INSTALL_DIR)"
	@mkdir -p $(INSTALL_DIR)
	@cp -r $(HEADERS) $(INSTALL_DIR)
	@{ \
		echo '#ifndef PTO_TILEOP_API_REVISION_HPP'; \
		echo '#define PTO_TILEOP_API_REVISION_HPP'; \
		echo ''; \
		echo '#define PTO_TILEOP_API_VERSION "$(VERSION)"'; \
		echo '#define PTO_TILEOP_API_SPEC_VERSION "0.58.6"'; \
		echo '#define PTO_TILEOP_API_REVISION "$(TILEOP_API_REVISION)"'; \
		echo '#define PTO_TILEOP_API_REVISION_IS_EXACT $(TILEOP_API_REVISION_IS_EXACT)'; \
		echo ''; \
		echo '#define PTO_TILEOP_API_HAS_LOCAL_B_KN_FIX 1'; \
		echo ''; \
		echo '#endif'; \
	} > $(INSTALL_DIR)/common/pto_tileop_api_revision.hpp
	@echo "Installation complete. Now you can use #include <$(LIBNAME)/header.h>"

uninstall:
	@echo "Removing $(INSTALL_DIR)"
	@rm -rf $(INSTALL_DIR)
	@echo "Uninstall complete"
