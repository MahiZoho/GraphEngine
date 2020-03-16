Installation/Deployment Steps for services:
===========================================

Download Latest build of the distdb Extension from
                                http://build/zoho/distributeddb/webhost/premaster/

unzip distributedb-installer.zip

The below files will be available in the zip and need to installed at respected nodes.

deploy_master_coordinator.sh
deploy_worker_node.sh
deploy_query_coordinator.sh
deploy_maintenance_coordinator.sh

./script-file-name PG_INSTALLATION_PATH  PG_DATA_PATH

    example: ./deploy_master_coordinator.sh /home/test/distdb/pgsql-10.4/ /home/test/distdb/pgdata/coordinator1/

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:PG_LIB_PATH

    example:  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/test/distdb/pgsql-10.4/lib/

You can know your PG_LIB_PATH by executing the below command at your postgres bin folder

    ./pg_config --libdir

Server will be started with the default postgres port. You can stop and start with the different port if neccessary.

Connect to all the components of Distributed Environment and Install the extensions using below commands:

```sql
CREATE EXTENSION distdb;
CREATE EXTENSION pglogical;
```

In primary coordinator one more extension has to be installed, pg_cron extension using the following command:

```sql
CREATE EXTENSION pg_cron;
```

For Devs
========

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
2. `pg_cron` extension which can be downloaded from their offical github page https://github.com/citusdata/pg_cron/releases (v1.2.0 is recommended)

(Both can be installed using `make PG_CONFIG='pg_config_path' install`)

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
```sh
make PG_CONFIG='pg_config_path' coverage_build
make PG_CONFIG='pg_config_path' installcheck
make PG_CONFIG='pg_config_path' PG_BUILDDIR='pg_build_dir' isolationcheck
make PG_CONFIG='pg_config_path' coverage_html
google-chrome coverage/index.html
```
Note: `gcov` and `lcov` packages must be installed.

**For more details on testing please refer to [test/README.md](test/README.md)**
