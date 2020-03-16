DistributedDB is an extension for PostgreSQL which is used to make a distributed database
on top of PostgreSQL database.

Getting Started
---------------
First we should create a cluster, follow the info given in [INSTALL.md](INSTALL.md) to create database nodes.
We assume that we already have set up 3 nodes, out of which one is the coordinator and the others are workers.
So cluster setup is

| hostname  | port | role        |
|-----------|------|-------------|
| localhost | 9800 | coordinator |
| localhost | 9801 | worker1     |
| localhost | 9802 | worker2     |

1. In coordinator, execute the following:
   (The extension will already be installed while running the scripts mentioned INSTALL.md)

    Add worker nodes using:
    ```sql 
    SELECT add_worker_node('localhost', 9801);
    SELECT add_worker_node('localhost', 9802);
    ```
    
    Once we have added the worker nodes, we can distribute already existing tables.

    In the coordinator. We can make the tables either a *distributed table* or a *reference table*.

    ### Distributed Table

    ![partitionMethod](https://git.csez.zohocorpin.com/opensource/distributedDB/raw/premaster/design-docs/partitionMethod.png)

    Each level uses a partitionKey ( column of the local table) for partitioning of the previous level shard. 

    Only the last level shards are physically present. All other intermediate shards are imaginary. Below the last level shards, further partitioning can be done using pg_partition. 

    A Local table can be fragmented/sharded into multiple partitions (last level shards ) and these partitions are
    distributed among worker nodes using the function `distribute_table()`
    
    >**distribute_table('tableName', 'partition_info_json', 'colocate_with' optional, 'foregin_table_info')**
     
    ### Arguments

    * `tableName`: the name of the table to be distributed
    * `partition_info_json`: Info regarding how to partition the table, which is explained in detail below
    * `colocate_with` (optional): is the name of the relation the new relation is planning to be colocated with. If the colocation criteria is not satisfied, function will error out. Default value is 'default', and it will put the relation into default colocation group based on shard_count and distribution column type. 'none' is provided to put the relation in a new seperate colocation group. 
    * `foreign_table_info` (optional): the shards will be created as foreign shards.
    -------------------
    `partition_info_json` should contain:
    We support `hash` based and `range` based partition, the following are common to both partition types.
    * `partition_key(s)` : column on which distribution is done
    * `partition_type`   : can be `hash` or `range`.
    * `is_pg_partitioned`: can be `true` or `false`. All the below levels of pg_partitioned levels should be pg_partitoned.
    * `partition_key_manipulator` (optional) : If we want to modify the partition key value before doing hash or range partition calculation on that, we should specify _partition_key_manipulator_ function.(An example could be SAS id, let's say we allocate 1Million to 2Million to one org, and 3 to 4 for another and so on. Then this partition key manipulator function basically floors 1 to 2 million value to just 1 million, so the org's data goes into a single shard, and similarly floors 2 to 3 million to just 2 million)
    * `next` (optional) : If we need multi level partitioning, we add *partition_info_json* as next key for the parent level, this can be repeated as many levels as required.


    Depending upon the `partition_type` more keys are needed, as explained below:
 
    **if partition_type is hash**
    * `no_of_shards` (optional): number of the shards required for the table(default value will be taken if this field is not provided)

    **if partition type is range**
    * `interval` : Specifies how much range of values should be stored in a single shard, value depends on the partition key type. If partition key is timestamp, we can give *interval* like `1 day`, `2 weeks` and so on. If partition key is integer, it can be any integer like `0`, `1001` or `-423123`..
    * `startval` (optional) : Starting value of the data from which shards are to be created. Default value is `1` for *integer* partition key. For *timestamp* partition key it will be current day/current month/current year based on the interval value(1 day/1 month/1 year).
    * `endval` (optional) : Upto this value, shards will be created. If endval is given, premake will be ignored.
    * `premake`(optional) : Number of shards to be pre-created initially. Default value: 5. Will be used while range_extend() and run_range_maintenance().
    * `retention` (optional) : The number/time-interval  of previous  row entries have to be maintained. It will be used during range_retention () and run_range_maintenance() udf functions. Default value :0 (unlimited retention).
    * `schedule` (optional) : The run_range_maintenance() function  have to be periodically  executed at background to perform data retention and shard pre-make. This is just a cron job. The schedule option should specified as per cron pattern.
    
    **Example: 1**
    Single level partition, hash based.
    
    Distributing the table 'person' with partition key being the 'id' column

    ```sql
    SELECT distribute_table(
        'person',
        '{
            "partition_key": ["id"],
            "partition_type": "hash",
            "no_of_shards": 32
        }'
    );
    ```
    ![ReadMeDistEx1](https://git.csez.zohocorpin.com/opensource/distributedDB/raw/premaster/design-docs/ReadMeDistEx1.png)

    **Example: 2**
    Single level partition, range based

    Distributing the table 'devices' with partition key being the 'id' column
    
    ```sql
    SELECT distribute_table(
        'devices',
        '{
            "partition_key":["id"],
            "partition_type":"range",
            "interval":"5",
            "startval":"1" ,
            "endval":"43"
        }'
    );
    ```
    ![ReadMeDistEx1](https://git.csez.zohocorpin.com/opensource/distributedDB/raw/premaster/design-docs/ReadMeDistEx2.png)

    For the above command, 9 shards will be created for **devices** table as shard_1 for id values from 1 to 5, shard_2 for id values 
    from 6 to 10 ... shard_9 for values from 41 to 45 

    **Example: 3**
    Two level partitioning, first level hash based and second level range based with timestamp partitioning.

    Distributing a table *iot_data* with two level partitions, top level being the *deviceid* and next level
    on *recorded_time*. Here we've created 32 top level shards, and each one sharded again for each day, so 32x30 shards will be created by the following command.
    ```sql
    SELECT distribute_table(
        'iot_data',
        '{
            "partition_key":["deviceid"],
            "partition_type":"hash",
            "no_of_shards":32,
            "next": {
                "partition_key": ["recorded_time"],
                "partition_type": "range",
                "interval" : "1 day",
                "startval" : "2019-09-21 00:00:00",
                "premake" : "30",
                "retention" : "30 days"
            }
        }'
    );
    ```
   ![ReadMeDistEx1](https://git.csez.zohocorpin.com/opensource/distributedDB/raw/premaster/design-docs/ReadMeDistEx3.png)

    ### Reference table
    In reference table, the given table will be duplicated in all of the worker nodes.
    
    It is usually done for small tables and which is very rarely updated. Those tables are also
    usually JOINed with other tables. Ideal candidate could be dimension tables.


    The following function is used to reference a table. 
    
    **reference_table(tableName)**

    * `tableName` should be the name of the table to has to be made as a reference table.

    **Example:**
    ```sql
    SELECT reference_table('order_status_enums');
    ```


    Now we can do SELECT and DML queries on the distributed table as well as reference tables.

    
Maintenance UDF Functions
-------------------------
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


* **SELECT undistribute_table(tableName)**

    * `tableName` undistributes the table ie., copy data from worker nodes into the coordinator, and drops the shards. In a nutshell, convert the distributed table back into local table

    **Example:**
    ```sql
    SELECT undistribute_table('person');
    ```

* **SELECT unreference_table(tableName)**

    * `tableName` should be the name of the table to be unreferenced, drop copies of the reference table from all of the nodes, and creates a local copy of that in the coordinator.

    **Example:**
    ```sql
    SELECT unreference_table('order_status_enums');
    ```    
* **range_retention( colocationid, level, use_up_to_key default false, up_to_key default '0')**
    
    * This function removes the shards where
                "shardmaxvalue  < (maxKeyUsed -retention)" for use_up_to_key = false,
                "shardmaxvalue  < up_to_key" for use_up_to_key = true.
    * maxKeyUsed is the maximum value of the partition key of the distributed table.


    **Example:**
    ```sql
    select range_retention(12,1,true,'2019-08-18 00:00:00');
    ```   
* **range_extend (colocationid,level,use_extend_to_key default false, extend_to_key default '0')**

   * Ensures premake number of shards is present after the maxKeyUsed. (for use_extend_to_key = false )
   * Shards are created upto extend_to_key. (for use_extend_to_key = true).


    **Example:**
    ```sql
    select range_extend(12,1,true,'2021-08-18 00:00:00');
    ``` 
* **run_range_maintenance (colocationid,level)**
  
    * This function is the combination of range_retention() and range_extend().


    **Example:**
    ```sql
    select run_range_maintenance(12,1);
    ``` 
* **range_schedule (colocationid,level,schedule)**

    * This function execute the run_range_maintenance() periodically in the background.


    **Example:**
    For every two months maintained. 
    ```sql
    select range_schedule(12,1,'0 0 */2 * * *');
    ``` 
* **range_unschedule (colocationid,level)**

    * To deactivate the schedule.


    **Example:**
    ```sql
    select range_unschedule(12,1);
    ``` 
    
Rebalancing Shards(Doesn't work as of now)
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

Configuring DistributedDB in postgresql.conf
-------------

* **To define the behaviour of asynchronous execution**
1. `distdb.maximum_connections_per_node` :  
    * Integer variable which defines the number of connections to be opened per workernode in a single execution. Default value is 4.
2.  `distdb.force_max_parallelization`  :  
    * Boolean variable which if set instructs to open one connection per task. Default value is false.
3.  `distdb.enable_session_connections`  :  
    * Set this variable to true if the user wants to keep the connections to be open till session ends if set to false, connections will close when the transaction is complete. Default value is true.

## Coordinator related Configurations.
In coordinator's postgresql.conf file add the following line:


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
    distdb.distributeNonEmptyTable = false
```
And restart PostgreSQL to take these things into effect.


DistributeDB MetaTables
-----------------------
1. dist_tables: Stores the info regarding which tables are distributed
2. dist_nodes: Info regarding the worker nodes
3. dist_shards: Info about all shards
4. dist_coordinators: stores the info of primary and query coordinators.
