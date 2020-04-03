#include "router.h"
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include "router_hal.h"

RoutingTableEntry table[1024];
int length_table = 0;
/*
  RoutingTable Entry 的定义如下：
  typedef struct {
    uint32_t addr; // 大端序，IPv4 地址
    uint32_t len; // 小端序，前缀长度
    uint32_t if_index; // 小端序，出端口编号
    uint32_t nexthop; // 大端序，下一跳的 IPv4 地址
  } RoutingTableEntry;

  约定 addr 和 nexthop 以 **大端序** 存储。
  这意味着 1.2.3.4 对应 0x04030201 而不是 0x01020304。
  保证 addr 仅最低 len 位可能出现非零。
  当 nexthop 为零时这是一条直连路由。
  你可以在全局变量中把路由表以一定的数据结构格式保存下来。
*/

/**
 * @brief 插入/删除一条路由表表项
 * @param insert 如果要插入则为 true ，要删除则为 false
 * @param entry 要插入/删除的表项
 * 
 * 插入时如果已经存在一条 addr 和 len 都相同的表项，则替换掉原有的。
 * 删除时按照 addr 和 len 匹配。
 */
void update(bool insert, RoutingTableEntry entry)
{
  // TODO:
  bool flag = false;
  if (insert)
  {
    entry.timestamp = HAL_GetTicks();
    for (int k = 0; k < length_table; k++)
    {
      if (table[k].addr == entry.addr && table[k].len == entry.len)
      {
        table[k] = entry;
        flag = true;
      }
    }
    if (!flag)
    {
      table[length_table] = entry;
      length_table++;
    }
  }
  else //delete
  {
    for (int i = 0; i < length_table; i++)
    {
      if (table[i].addr == entry.addr & table[i].len == entry.len)
      {
        for (int j = i; j < length_table; j++)
          table[j] = table[j + 1];
        length_table--;
      }
    }
  }
}
bool wangduan(uint32_t addr, RoutingTableEntry entry)
{
  uint32_t yu = 1;
  uint32_t count = 0;
  for (int i = 0; i < entry.len; i++)
  {

    count += yu << (i);
  }
  if ((addr & (count)) == (entry.addr & (count)))
    return true;
  else
    return false;
}

// int get_max(uint32_t addr, RoutingTableEntry entry)
// {
//   int count=0;
//   uint32_t yu = 1;
//   uint32_t count = 0;
//   for (int i = 0; i < entry.len; i++)
//   {
//     count += yu << (i);
//     if (addr & (count) == entry.addr & (count))
//     {
//       count++;
//       continue;
//     }
//     else
//     {
//       break;
//     }
//   }
//   return count;
// }
/**
 * @brief 进行一次路由表的查询，按照最长前缀匹配原则
 * @param addr 需要查询的目标地址，大端序
 * @param nexthop 如果查询到目标，把表项的 nexthop 写入
 * @param if_index 如果查询到目标，把表项的 if_index 写入
 * @return 查到则返回 true ，没查到则返回 false
 */
bool query(uint32_t addr, uint32_t *nexthop, uint32_t *if_index)
{
  // TODO:

  *nexthop = 0;
  *if_index = 0;
  int max_len = -1;
  int max_index = -1;
  for (int i = 0; i < length_table; i++)
  {
    if (wangduan(addr, table[i]))
    {
      if ((int)table[i].len > max_len)
      {
        max_len = table[i].len;
        max_index = i;
      }
    }
  }
  if (max_index >= 0)
  {
    *if_index = table[max_index].if_index;
    *nexthop = table[max_index].nexthop;
    return true;
  }
  else
    return false;
}
