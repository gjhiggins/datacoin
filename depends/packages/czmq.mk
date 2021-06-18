package=czmq
$(package)_version=3.0.2
$(package)_download_path=https://github.com/zeromq/czmq/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=8bca39ab69375fa4e981daf87b3feae85384d5b40cef6adbe9d5eb063357699a
$(package)_dependencies=zeromq

define $(package)_set_vars
$(package)_config_opts=--disable-shared --without-test_zgossip --without-makecert
$(package)_config_opts_mingw32=--enable-mingw
$(package)_config_opts_linux=--with-pic
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

