DistDB is an extension for PostgreSQL, which can be used to create a distributed 
database, using PostgreSQL.

How to install this?
--------------------
First we should create two or more PostgreSQL servers. One of the servers will be _the coordinator_ and all others will be _worker_ or _data nodes_.

For building and installing please refer [INSTALL.md](INSTALL.md)

Example
-------
1. Create three servers. One should be the coordinator.
2. In coordinator's postgresql.conf file add the following line:


    `shared_preload_libraries = 'distdb'`
* There are a few  extension specific settings that must be placed at the end of this file.
    ( Following 2 GUC variables are for Shared memory allocation, so give maximum possible values )
    * distdb.MaximumDistributedTable = '\<max number(count) of tables to be distributed\>'
    * distdb.MaximumDataNodes = '\<max number(count) of data nodes\>'
    
    * distdb.isPrimaryCoordinator = '\<true for primary coordinator, false for other cluster \>'
    * distdb.isQueryCoordinator = '\<true for query coordinator, false for other cluster \>'
    * distdb.isMaintenanceCoordinator = '\<true for maintenance coordinator, false for other cluster \>'

Refer below for sample:
```bash
    distdb.MaximumDistributedTable = 50
    distdb.MaximumDataNodes = 10
    distdb.isPrimaryCoordinator = true
    distdb.isQueryCoordinator = false
    distdb.isMaintenanceCoordinator = false
    distdb.shardCountForTopLevel = 64
    distdb.shardCountForNonTopLevel = 4
    distdb.Max_Sub_Partition_Levels = 4
```
And restart PostgreSQL to take these things into effect.

3. Run the worker1 in port 9801, and worker2 in port 9802.
4. In coordinator, execute the following:
    
    Install the extension using (extension need not be installed in workers):
    ```sql 
    CREATE EXTENSION distdb;
    ```

    Add worker nodes using:
    ```sql 
    SELECT add_worker_node('localhost', 9801);
    SELECT add_worker_node('localhost', 9802);
    ```

    Now let's create a simple table
    ```sql 
    CREATE TABLE person(
        id int primary key,
        name text
    );
    ```

    Now let's distribute the table using
    ```sql 
    SELECT distribute_table('person','{"partition_key":["id"],"partition_type":"hash","no_of_shards":2}');
    ```

    Now we can do SELECT and DML queries on this distributed table.

    Now let's create a simple table
    ```sql 
    CREATE TABLE customer(
        customer_id int primary key,
        name text
    );
    ```
    
    Now let's create a reference table using
    ```sql
    SELECT reference_table('customer');
    ```
    Now we can do SELECT and DML queries on this reference table.

    Now let's unreference the table
    ```sql
    SELECT unreference_table('customer');
    ```

UDF Functions
-------------
* **add_worker_node('url', 'port')**

    Used to add worker node into the distributed database.

    **Example:**
    ```sql
    SELECT add_worker_node('localhost', 9801);
    ```
* **remove_worker_node('url', 'port', 'shouldRebalance')**

    Used to remove worker node from distributed database.

    **Example:**
    ```sql
    SELECT remove_worker_node('localhost', 9801);
    ```
    If there are shards in that node, we may need to rebalance and remove the node.

* **distribute_table('tableName', 'sub_partition_json', 'colocate_with')**
     
    Used to distribute a local table.

    * `tableName` should be the name of the table to be distributed
    * `json` should have partition_key,partition_type,(partition_key_manipulator function(if needed),inner json(if needed)).
    * `colocate_with` is the name of the relation the new relation is planning to be colocated with. If the colocation criteria is not satisfied, function will error out. Default value is 'default', and it will put the relation into default colocation group based on shard_count and distribution column type. 'none' is provided to put the relation in a new seperate colocation group. 
    -------------------
    json should contain:

    * `partition_key(s)` : column on which distribution is done
    * `partition_type`   : can be `hash` or `range` or `function`. Now `hash` is only supported
    * `no_of_shards` (optional): number of the shards required for the table
    * `interval` (optional) : interval for the range partitioning
    * `partition_function` (optional) : partition function for function based partitioning
    * `partition_key_manipulator` (optional) : This function is applicable for any partition_type
    * `next` (optional) : if inner sub_partition is needed,it should be in a json format
    **Example:**
    ```sql
    SELECT distribute_table('person','{"partition_key":["id"],"partition_type":"hash","no_of_shards":2,"next":{"partition_key":["name"],"partition_type":"hash","no_of_shards":5}}');
    ```

* **reference_table(tableName)**

    * `tableName` should be the name of the table to be referenced

    **Example:**
    ```sql
    SELECT reference_table('customer');
    ```

MetaTables
----------
1. dist_tables: Stores the info regarding which tables are distributed
2. dist_nodes: Info regarding the worker nodes
3. dist_shards: Metadata about all shards.
4. dist_coordinators: stores the info of primary and query coordinators.

Rebalancing Shards
------------------
* **Add new nodes and rebalance shards**

1. Add new worker nodes using:

    ```sql
    SELECT add_worker_node('localhost', 9803);
    SELECT add_worker_node('localhost', 9804);
    ```

2. To rebalance shards of a distributed table (eg. persons) among nodes

    ```sql
    SELECT rebalance_shards('persons');
    ```

After rebalancing no. of shards of the specified table in each node will be almost equal.

* **Remove node and rebalance shards**

1. Remove and rebalance shards by executing:

    ```sql
    SELECT remove_worker_node('localhost', 9804, true);
    ```

Now it will transfer all the shards in the specified node to other nodes.

### Add shards to the existing distributed table

* **SELECT add_shards(tableName, no_of_shards, workerNodeArray[] (optional))**

    * `tableName` should be the name of the table we need to add shards
    * `no_of_shards` is the number of extra shards to be added
    * `workerNodeArray[]`(optional) : array of worker nodes where we need to place the shards

    **Example:**
    ```sql
    SELECT add_shards('person', 4, '{3,4}');
    ```

### Split Existing shard

* **SELECT split_shard(shardId, no_of_splits, splitCriteriaJson[] (optional))**

    * `shardId` corresponds to the shard to be split
    * `no_of_splits` is the no of parts the given shard is to be split into
    * `splitCriteriaJson[]` (optional) : if provided, should contain shardminvalue, shardmaxvalue, destworkernode
        for all the splits. If not provided shards will be split uniformly.

    **Example:**
    ```sql
    SELECT split_shards(1160, 2,
                            '[{"shardminvalue": "-1342177280",
                               "shardmaxvalue": "-1207959553",
                               "destworkernode": 0},
                              {"shardminvalue": "-1207959552",
                               "shardmaxvalue": "-1073741825",
                               "destworkernode": 4}]');
    ```
### delete rows from dist_tables, dist_shards and removes shards from workers

* **SELECT undistribute_table(tableName)**

    * `tableName` should be the name of the table to be deleted

    **Example:**
    ```sql
    SELECT undistribute_table('person');
    ```

* **SELECT unreference_table(tableName)**

    * `tableName` should be the name of the table to be unreferenced

    **Example:**
    ```sql
    SELECT unreference_table('customer');
    ```    

Sync Metadata to Query Coordinators
------------------------------------
* **Add Query Coordinators and copy metadata to them**
1. Add Primary Coordinator

    ```sql
    SELECT add_primary_coordinator('coordinator_hostname' , port);
    ```
2. Add Query Coordinators

    ```sql
    SELECT add_query_coordinator('query_coordinator1_hostname', port1);
    SELECT add_query_coordinator('query_coordinator2_hostname', port2);
    ```
3. Sync metadata to Query Coordinators

    ```sql
    SELECT sync_metadata();
    ```

GUC Variables
-------------

* **To define the behaviour of asynchronous execution**
1. `distdb.maximum_connections_per_node` :  
    * Integer variable which defines the number of connections to be opened per workernode in a single execution. Default value is 4.
2.  `distdb.force_max_parallelization`  :  
    * Boolean variable which if set instructs to open one connection per task. Default value is false.
3.  `distdb.enable_session_connections`  :  
    * Set this variable to true if the user wants to keep the connections to be open till session ends if set to false, connections will close when the transaction is complete. Default value is true.