Compilation
===========

First get the absolute path to pg_config, which will be in pg/bin folder, let's
call it `pg_config_path`.

An example path can be 

    /home/local/ZOHOCORP/my_user_name/postgres-10.3/bin/pg_config

## Steps:

**1. Build**

 * Execute `make PG_CONFIG='pg_config_path'` (quotes around `pg_config_path` is recommended)
    
**2. Install**
 * Execute `make PG_CONFIG='pg_config_path' install`

That's it, the extension is installed.

Dependencies
------------
1. Modified `pglogical` which can be downloaded from https://git.csez.zohocorpin.com/opensource/pglogical/tree/premaster

For Devs
========

Running regression test
------------------------
1. Do `make PG_CONFIG='pg_config_path' installcheck` for running regression tests.

Running code coverage check
---------------------------
For code coverage to be run, the extension must be compiled with flags for coverage,
for that follow these steps.

1. Build for coverage analysis by executing `make PG_CONFIG='pg_config_path' coverage_build`
2. Now run any checks you want, the coverage stats will be automatically collected. It's recommended to run regression test, so that we can know how much our test suite covers the code.

   * So execute `make PG_CONFIG='pg_config_path' installcheck`
   * Also run isolation test, `make PG_CONFIG='pg_config_path' PG_BUILDDIR='pg_build_dir' isolationcheck`
   * Note: If your module is not well covered, please add test cases in to the test suite. (Please refer to [test/README.md](test/README.md) )
3. Now let's generate the coverage stats in html format.
   * Execute `make PG_CONFIG='pg_config_path' coverage_html`
   * This command will generate HTML files for the coverage stats, in ./coverage/ folder. Open ./coverage/index.html in any browser to see the coverage stats.
4. Coverage cleanup
   * If you're done with coverage tests, the clean up for that can be done by running `make PG_CONFIG='pg_config_path' coverage_clean`

**Short Version**

    make PG_CONFIG='pg_config_path' coverage_build
    make PG_CONFIG='pg_config_path' installcheck
    make PG_CONFIG='pg_config_path' PG_BUILDDIR='pg_build_dir' isolationcheck
    make PG_CONFIG='pg_config_path' coverage_html
    google-chrome coverage/index.html
Note: `gcov` and `lcov` packages must be installed.

**For more details on testing please refer to [test/README.md](test/README.md)**
