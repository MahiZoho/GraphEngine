/**
 * A (persistent) Redis API built using the rocksdb backend.
 * Implements Redis Lists as described on: http://redis.io/commands#list
 *
 * @throws All functions may throw a RedisListException
 *
 * @author Deon Nicholas (dnicholas@fb.com)
 * Copyright 2013 Facebook
 */

#ifndef ROCKSDB_LITE
#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include "rocksdb/db.h"
#include "redis_list_iterator.h"
#include "redis_list_exception.h"

namespace rocksdb {

/// The Redis functionality (see http://redis.io/commands#list)
/// All functions may THROW a RedisListException
class RedisLists {
 public: // Constructors / Destructors
  /// Construct a new RedisLists database, with name/path of db.
  /// Will clear the database on open iff destructive is true (default false).
  /// Otherwise, it will restore saved changes.
  /// May throw RedisListException
  RedisLists(const std::string& db_path,
             Options options, bool destructive = false);

  RedisLists(const std::string& db_path, DB* db, const WriteOptions& write_options, const ReadOptions& read_options);

  virtual ~RedisLists() 
  {
    std::cout << "Inside RedisLists Destructor..." << std::endl;
  }

 public:  // Accessors
  /// The number of items in (list: key)
  int Length(const std::string& key);

  /// Search the list for the (index)'th item (0-based) in (list:key)
  /// A negative index indicates: "from end-of-list"
  /// If index is within range: return true, and return the value in *result.
  /// If (index < -length OR index>=length), then index is out of range:
  ///   return false (and *result is left unchanged)
  /// May throw RedisListException
  bool Index(const std::string& key, int32_t index,
             std::string* result);

  /// Return (list: key)[first..last] (inclusive)
  /// May throw RedisListException
  std::vector<std::string> Range(const std::string& key,
                                 int32_t first, int32_t last);

  /// Prints the entire (list: key), for debugging.
  void Print(ColumnFamilyHandle* column_family, const std::string& key);
  Status getOriginalData(ColumnFamilyHandle* column_family, const std::string& key, std::string &value);
  Status getListData(ColumnFamilyHandle* column_family, const std::string& key, std::vector<std::string> &datavector);
  // Status PrintList(const std::string& key);

 public: // Insert/Update
  /// Insert value before/after pivot in (list: key). Return the length.
  /// May throw RedisListException
  Status InsertBefore(const std::string& key, const std::string& pivot,
                   const std::string& value);
  Status InsertAfter(const std::string& key, const std::string& pivot,
                  const std::string& value);

    /// Push / Insert value at beginning/end of the list. Return the length.
  /// May throw RedisListException
  // Status PushLeftStatus(const std::string& key, const std::string& value);
  // int PushLeft(const std::string& key, const std::string& value);
  Status PushLeft(ColumnFamilyHandle* column_family, const std::string& key, const std::string& value);
  Status PushRight(const std::string& key, const std::string& value);

  /// Set (list: key)[idx] = val. Return true on success, false on fail
  /// May throw RedisListException
  bool Set(const std::string& key, int32_t index, const std::string& value);

 public: // Delete / Remove / Pop / Trim
  /// Trim (list: key) so that it will only contain the indices from start..stop
  /// Returns true on success
  /// May throw RedisListException
  bool Trim(const std::string& key, int32_t start, int32_t stop);

  /// If list is empty, return false and leave *result unchanged.
  /// Else, remove the first/last elem, store it in *result, and return true
  bool PopLeft(const std::string& key, std::string* result);  // First
  bool PopRight(const std::string& key, std::string* result); // Last

  /// Remove the first (or last) num occurrences of value from the list (key)
  /// Return the number of elements removed.
  /// May throw RedisListException
  int Remove(const std::string& key, int32_t num,
             const std::string& value);
  int RemoveFirst(const std::string& key, int32_t num,
                  const std::string& value);
  int RemoveLast(const std::string& key, int32_t num,
                 const std::string& value);

  static Status getCommonData(const std::string& data1, const std::string& data2, std::vector<std::string> &commondatavector) {
    Status s;
    try{
      std::string smalldata;
      std::string largedata;
      std::vector<std::string> smalldatavector;
      int data1_size = static_cast<int>(data1.size()); 
      int data2_size = static_cast<int>(data2.size());

      if(data1_size == 0 || data2_size == 0){
        return s;
      }
      RedisListIterator it1(data1);
      RedisListIterator it2(data2);
      if(it1.Length() < it2.Length()){
        smalldata = data1;
        largedata = data2;
      }else{
        smalldata = data2;
        largedata = data1;
      }
    
      // std::cout << "=== Populating small list into Vector === " << std::endl;
      Slice elem;
      for (RedisListIterator smalldataIt(smalldata); !smalldataIt.Done(); smalldataIt.Skip()) {
        smalldataIt.GetCurrent(&elem);
        smalldatavector.push_back(elem.ToString());
      }

      //Not doing sort by default.... usually our list data will be minimal...so we will not be benefited with sort....
      // Below Math equation is the Trade-Off to determine when to sort and when not to sort..
      // if((static_cast<int>(log(smallvector_size)/log(2)))*(smallvector_size+largevector_size) < (smallvector_size*largevector_size)){
      //       std::sort (smalldatavector.begin(), smalldatavector.end());
      //       Write Our Own Find Function that uses Binary Search...because std::find will do a linear search
      // }else{
      //         Existing Code....
      // }

      // std::cout << "===== Iterating Large list and putting common data into common vector ===== " << std::endl;
      std::string data;
      for (RedisListIterator largedataIt(largedata); !largedataIt.Done(); largedataIt.Skip()) 
      {
        largedataIt.GetCurrent(&elem);
        data = elem.ToString();
        if ( std::find(smalldatavector.begin(), smalldatavector.end(), data) != smalldatavector.end() )
        {
          commondatavector.push_back(data);
        }
      }
    }catch(std::exception& e){
      s = Status::ZlabsError("Error while finding CommonData!\n");
    }
    return s;
  }


//   static int getIndex(std::string &data, const Slice &sliceElem)
//   {
//     std::string elem = sliceElem.ToString();

//     RedisListIterator it(data);
//     int currLength = it.Length();

//     int min = 0;
//     int max = currLength-1;
//     int mid;
//     std::string midElem;
//     int midElemCompareResult;
    
//     while (min <= max){
//       mid = min + (max-min)/2;
//       // midElem = getIndexData(it, mid);
//       getIndexData(it, mid, midElem);
//       midElemCompareResult = elem.compare(midElem);
//       if(midElemCompareResult == 0){
//           return mid;
//       }else if(midElemCompareResult < 0){
//           min = mid+1;
//       }else{
//           max = mid-1;
//       }
//     }
//     return -1;
//   }

// static void sort(std::string &data){
//   std::cout << "Inside sort 11111..." << std::endl;
//     RedisListIterator it(data);
//     // std::string sorteddata(getIndexData(it, 0, true));
//     std::string sorteddata;
//     getIndexData(it, 0, sorteddata, true);

//     Slice elem;
//     while(!it.Done()) {
//       it.GetCurrent(&elem);
//       std::cout << "ITEM " << elem.ToString() << std::endl;
//       int pos = getPosition(sorteddata, elem);
//       if(pos > -1){
//           constructData(sorteddata, elem.ToString(), pos);
//       }
//       it.Skip();
//     }
//     data = sorteddata;
    
//     std::cout << "Inside sort 2222..." << std::endl;
//     std::cout << "=== Inside Sort Function...Printing Sorted Small List === " << std::endl;
//     for (RedisListIterator resultItr(data); !resultItr.Done(); resultItr.Skip()) {
//       resultItr.GetCurrent(&elem);
//       std::cout << "ITEM " << elem.ToString() << std::endl;
//     }
//     std::cout << "----------------------------------------------------------" << std::endl;
// }

// static int getPosition(std::string &data, const Slice &sliceElem){
//     std::string elem = sliceElem.ToString();

//     RedisListIterator it(data);
//     int currLength = it.Length();
//     // std::string firstElem = getIndexData(it, 0);
//     // std::string lastElem = getIndexData(it, currLength-1);
//     std::string firstElem;
//     getIndexData(it, 0, firstElem);
//     std::string lastElem;
//     getIndexData(it, currLength-1, lastElem);
//     std::string midElem;

//     int pos;
//     int firstElemCompareResult = elem.compare(firstElem);
//     int lastElemCompareResult = elem.compare(lastElem);
//     int midElemCompareResult;

//     if(firstElemCompareResult == 0 || lastElemCompareResult == 0){
//         pos = -1;
//     }else if(firstElemCompareResult < 0){
//         pos = 0;
//     }else if(lastElemCompareResult > 0){
//         pos = currLength;
//     }else{
//         int mid;
//         int min = 0;
//         int max = currLength-1;
    
//         while (min <= max){
//             mid = min + (max-min)/2;
//             getIndexData(it, mid, midElem);
//             midElemCompareResult = elem.compare(midElem);
//             if(midElemCompareResult == 0){
//                 return -1;
//             }else if(midElemCompareResult < 0){
//                 min = mid+1;
//             }else{
//                 max = mid-1;
//             }
//         }
//         return min;
//     }
//     return pos;
// }

// static void constructData(std::string &sorteddata, std::string elem, const int &pos){
//   RedisListIterator it(sorteddata);
//   int length = it.Length();
//   std::string prefix = GetSubString(it, 0, pos-1);
//   std::string suffix = GetSubString(it, pos, length-1);
//   // sorteddata.erase(0, sorteddata.size());
//   sorteddata = prefix + elem + suffix;
// }

// static void getIndexData(RedisListIterator &it, int32_t index, std::string &result, bool header=false) {
//   // Handle REDIS negative indices (from the end); fast iff Length() takes O(1)
//   // std::string result;
//   if (index < 0) {
//     index = it.Length() - (-index);  //replace (-i) with (N-i).
//   }

//   // Iterate through the list until the desired index is found.
//   int curIndex = 0;
//   while(curIndex < index && !it.Done()) {
//     ++curIndex;
//     it.Skip();
//   }

//   // If we actually found the index
//   if (curIndex == index && !it.Done()) {
//     Slice elem;
//     if(header){
//       it.GetCurrentWithSizeHeader(&elem);
//     }else{
//       it.GetCurrent(&elem);
//     }
//     result = elem.ToString();
//   } 
//   // return result;
// }

// static std::string GetSubString(RedisListIterator &it, const int &start, const int &end){
//     std::string substring;
//     std::string data;
//     if(start > -1 && end < it.Length()){
//       int index = start;
//       while(index <= end){
//         getIndexData(it, index, data, true);
//         substring = substring+data;
//         index++;
//       }
//     }
//     return substring;
// }

 private: // Private Functions
  /// Calls InsertBefore or InsertAfter
  Status Insert(const std::string& key, const std::string& pivot,
             const std::string& value, bool insert_after);
 private:
  std::string db_name_;       // The actual database name/path
  WriteOptions put_option_;
  ReadOptions get_option_;

  /// The backend rocksdb database.
  /// Map : key --> list
  ///       where a list is a sequence of elements
  ///       and an element is a 4-byte integer (n), followed by n bytes of data
  //std::unique_ptr<DB> db_;
  DB* db_;
};

} // namespace rocksdb
#endif  // ROCKSDB_LITE
