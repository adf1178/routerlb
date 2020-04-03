#include "router_hal.h"
#include "rip.h"
#include "router.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

extern bool validateIPChecksum(uint8_t *packet, size_t len);
extern void update(bool insert, RoutingTableEntry entry);
extern bool query(uint32_t addr, uint32_t *nexthop, uint32_t *if_index);
extern bool forward(uint8_t *packet, size_t len);
extern bool disassemble(const uint8_t *packet, uint32_t len, RipPacket *output);
extern uint32_t assemble(const RipPacket *rip, uint8_t *buffer);
extern RoutingTableEntry table[1024];
extern int length_table;
uint8_t packet[2048];
uint8_t output[2048];
// 0: 10.0.0.1
// 1: 10.0.1.1
// 2: 10.0.2.1
// 3: 10.0.3.1
// 你可以按需进行修改，注意端序
in_addr_t addrs[N_IFACE_ON_BOARD] = {0x0100000a, 0x0101000a, 0x0102000a, 0x0103000a};
uint32_t compute_jiaoyanhe(uint8_t *packet)
{
  packet[10] = 0;
  packet[11] = 0;
  uint32_t sum = 0;
  int head_length = packet[0] & (0xf);
  for (int i = 0; i < 4 * head_length; i += 2)
  {
    long temp = 0, temp2 = (long)packet[i];

    temp += temp2 * 256;
    sum += temp + packet[i + 1];
  }
  int high_16 = (sum >> 16);
  while (high_16)
  {
    sum = sum & (0xffff);
    sum += high_16;
    high_16 = (sum >> 16);
  }
  sum = (~sum) & 0xffff;
  return sum;
  // int ten = sum & (0xff00);
  // int eleven = sum & (0xff);
  // packet[10] = ten >> 8;
  // packet[11] = eleven;
}
uint32_t swap_endian(uint32_t val)
{
  val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
  return (val << 16) | (val >> 16);
}
uint32_t len_to_mask(uint32_t len)
{
  uint32_t mask_small_end = 0;
  uint32_t tool_yu = 0x80000000;
  //先构造小端序mask
  //然后把小端转成大端返回
  for (int k = 0; k < len; k++)
  {
    mask_small_end += tool_yu;
    tool_yu = tool_yu >> 1;
  }
  uint32_t mask_big_end = swap_endian(mask_small_end);
  return mask_big_end;
}

uint32_t mask_to_len(uint32_t mask)
{
  //这里mask传进来是大端
  //所以不一定是连续个1
  uint32_t mask_small_end = swap_endian(mask);
  uint32_t len = 0;
  uint32_t tool_yu = 0x80000000;
  while (mask_small_end & tool_yu)
  {
    mask_small_end = mask_small_end << 1;
    len++;
  }
  return len;
}
void dabaofasong()
{
  uint8_t buffer[1024];
  RipPacket rip;
  rip.numEntries = length_table;
  rip.command = 2; // 发回去response,我的整个路由表
  for (int j = 0; j < length_table; j++)
  {
    //std::cout << std::hex << table[j].addr << ' ' << table[j].metric << ' ' << swap_endian(table[j].metric) << "next hop:" << table[j].nexthop << ' ' << table[j].if_index << std::endl;
    rip.entries[j].addr = table[j].addr;
    rip.entries[j].mask = len_to_mask(table[j].len);
    rip.entries[j].nexthop = 0;
    rip.entries[j].metric = table[j].metric;
  }
  uint32_t len = assemble(&rip, buffer + 28);
  uint32_t total_len = len + 28;
  buffer[0] = 0x45;                        //版本号   首部长度
  buffer[1] = 0;                           //tos
  buffer[2] = (total_len & (0xff00)) >> 8; //总长度高八位
  buffer[3] = total_len & (0xff);          //总长度低八位
  buffer[4] = 1999;                        //标识
  buffer[5] = 0627;                        //标识
  buffer[6] = 0;                           //标志位
  buffer[7] = 0;                           //片偏移
  buffer[8] = 255;                         //ttl
  buffer[9] = 17;                          //协议 表示udp
  //校验和 先写0
  buffer[10] = 0;
  buffer[11] = 0;
  //12~15是源IP  先写0
  buffer[12] = 0;
  buffer[13] = 0;
  buffer[14] = 0;
  buffer[15] = 0;
  //16~19 是目的IP  组播IP
  buffer[16] = 224;
  buffer[17] = 0;
  buffer[18] = 0;
  buffer[19] = 9;

  //udp
  buffer[20] = 0x02;
  buffer[21] = 0x08;
  buffer[22] = 0x02;
  buffer[23] = 0x08;
  buffer[24] = ((len + 8) & (0xff00)) >> 8; //总长度高八位
  buffer[25] = (len + 8) & (0xff);
  buffer[26] = 0;
  buffer[27] = 0;

  for (int i = 0; i < 4; i++)
  {
    uint8_t mac[6] = {1, 0, 0x5e, 0, 0, 9};
    buffer[12] = 10;
    buffer[13] = 0;
    buffer[14] = i;
    buffer[15] = 1;
    uint32_t jiaoyanhe = compute_jiaoyanhe(buffer);
    int ten = jiaoyanhe & (0xff00);
    int eleven = jiaoyanhe & (0xff);
    buffer[10] = ten >> 8; //首部校验和
    buffer[11] = eleven;   //首部校验和
    //std::cout << "现在在打包发送,端口为" << i << std::endl;
    HAL_SendIPPacket(i, buffer, len + 28, mac); //rip组播IP地址224.0.0.9
  }
}
int main(int argc, char *argv[])
{
  int res = HAL_Init(1, addrs);
  if (res < 0)
  {
    return res;
  }

  // Add direct routes
  // For example:
  // 10.0.0.0/24 if 0
  // 10.0.1.0/24 if 1
  // 10.0.2.0/24 if 2
  // 10.0.3.0/24 if 3
  for (uint32_t i = 0; i < N_IFACE_ON_BOARD; i++)
  {
    RoutingTableEntry entry = {
        .addr = addrs[i] & len_to_mask(24), // big endian
        .len = 24,                          // small endian
        .if_index = i,                      // small endian
        .nexthop = 0,                       // big endian, means direct
        .metric = swap_endian(1)
    };
    update(true, entry);
  }

  uint64_t last_time = 0;
  uint64_t last_time_stampexam = 0;
  while (1)
  {
    uint64_t time = HAL_GetTicks();
    if (time > last_time + 30 * 1000)
    {
      // What to do?
      dabaofasong();
      last_time = time;

      printf("Timer\n");
    }
    if(time > last_time_stampexam + 10*1000)//每10秒检查一次本地路由表项的时间戳
    {
      last_time_stampexam = time;
      for (int i=0;i<length_table;i++)
      {
        if(table[i].timestamp<(HAL_GetTicks()-180*1000))//如果超过了180秒
        {
	  if(table[i].nexthop!=0)
          table[i].metric = swap_endian((uint32_t)16);
        }
        if(table[i].timestamp<(HAL_GetTicks()-300*1000))
        {
	  if(table[i].nexthop!=0)
          update(false,table[i]);
        }
      }
    }
    //轮询
    int mask = (1 << N_IFACE_ON_BOARD) - 1;
    macaddr_t src_mac;
    macaddr_t dst_mac;
    int if_index;
    res = HAL_ReceiveIPPacket(mask, packet, sizeof(packet), src_mac,
                              dst_mac, 1000, &if_index); //超时为1s
    if (res == HAL_ERR_EOF)
    {
      break;
    }
    else if (res < 0)
    {
      return res;
    }
    else if (res == 0)
    {
      // Timeout
      continue;
    }
    else if (res > sizeof(packet))
    {
      // packet is truncated, ignore it
      continue;
    }

    if (!validateIPChecksum(packet, res))
    { //这里的目的是检查校验和是否正确
      printf("Invalid IP Checksum\n");
      continue;
    }
    in_addr_t src_addr, dst_addr;
    // extract src_addr and dst_addr from packet
    // big endian
    src_addr = *(uint32_t *)(packet + 12);
    dst_addr = *(uint32_t *)(packet + 16);

    bool dst_is_me = false;
    bool dst_is_multi = false;
    //std::cout << "收到一个包 IP" << std::hex << dst_addr << ' ' << if_index << std::endl;
    for (int i = 0; i < N_IFACE_ON_BOARD; i++)
    {
      if (memcmp(&dst_addr, &addrs[i], sizeof(in_addr_t)) == 0)
      { //检查路由器是否是自己的ip
        dst_is_me = true;
        break;
      }
    }
    uint32_t multicast_adr = ((uint32_t)9 << 24) + 224;
    if (dst_addr == multicast_adr)
      dst_is_multi = true;

    // TODO: Handle rip multicast address?

    if (dst_is_me || dst_is_multi)
    { //3a
      // TODO: RIP?
      RipPacket rip;
      //std::cout << "收到RIP" << std::endl;
      //res是packet的长度
      if (disassemble(packet, res, &rip))
      { // 3a.1 检查是否是合法的 RIP 包，可以用你编写的 disassemble 函数检查并从中提取出数据
        //这里的rip内容全部都是大端序，包括metirc
        if (rip.command == 1)
        { // 3a.3 如果是 Request 包，就遍历本地的路由表，构造出一个 RipPacket 结构体，
          // request
          RipPacket resp;
          // TODO: fill resp  根据rip和本地的路由表来填充resp
          if (rip.numEntries == 1 && rip.entries[0].metric == swap_endian((uint32_t)16)) //以前已经判断过，只要command =1 那么family一定要是0
          {                                                       //这种情况就把本地所有0路由表都发过去
            resp.numEntries = length_table;
            resp.command = 2; //收到一个request 发回去response
            for (int j = 0; j < length_table; j++)
            {
              resp.entries[j].addr = table[j].addr;

              resp.entries[j].mask = len_to_mask(table[j].len);
              resp.entries[j].nexthop = 0;
              resp.entries[j].metric = table[j].metric;
            }
          }
          else //否则只把想要的发过去
          {
            uint32_t count = 0;
            resp.numEntries = rip.numEntries;
            for (int l = 0; l < rip.numEntries; l++)
            {
              for (int m = 0; m < length_table; m++) //遍历找到addr 和 网段 都相同的table和entries
              {
                if (rip.entries[l].addr == table[m].addr && mask_to_len(rip.entries[l].mask) == table[m].len)
                {
                  resp.entries[l].addr = table[m].addr;             //大端
                  resp.entries[l].mask = len_to_mask(table[m].len); //大端
                  resp.entries[l].nexthop = 0;
                  resp.entries[l].metric = table[m].metric;
                  break; //这个table已经找到一个rip.entries和它相等，那么看下一个table
                }
                //如果这个table[m]没有rip.entries与之对应，那么也把它发回去，但是metirc是16
                resp.entries[l].addr = rip.entries[l].addr; //大端
                resp.entries[l].mask = rip.entries[l].mask; //大端
                resp.entries[l].nexthop = 0;
                resp.entries[l].metric = swap_endian((uint32_t)16);
              }
            }
          }

          // assemble
          uint32_t rip_len = assemble(&resp, &output[20 + 8]); //rip len 是解构出来的rip数据包的长度
          // IP
          output[0] = 0x45; //版本和 首部长度
          output[1] = 0;    //tos
          uint32_t total_len2 = rip_len + 28;
          output[2] = (total_len2 & (0xff00)) >> 8; //总长度高八位
          output[3] = total_len2 & (0xff);          //总长度低八位
          output[4] = 1999;                         //标识
          output[5] = 0627;                         //标识
          output[6] = 0;                            //标志位
          output[7] = 0;                            //片偏移
          output[8] = 255;                          //ttl
          output[9] = 17;                           //协议 表示udp
          output[10] = 0;                           //首部校验和 先写0
          output[11] = 0;                           //首部校验和 先写0
          // 从if_index口发出去
          output[12] = 10;
          output[13] = 0;
          output[14] = if_index;
          output[15] = 0;
          //因为是发给自己的，所以应该发回去   接受的源ip就是自己的目的ip
          output[16] = src_addr & (0xff);
          output[17] = src_addr & (0xff00) >> 8;
          output[18] = src_addr & (0xff0000) >> 16;
          output[19] = src_addr & (0xff000000) >> 24;
          // ...
          // UDP
          // port = 520
          output[20] = 0x02;
          output[21] = 0x08;
          output[22] = 0x02;
          output[23] = 0x08;
          output[24] = 0;
          output[25] = 0;
          output[26] = 0;
          output[27] = 0;
          uint32_t jiaoyanhe = compute_jiaoyanhe(output);
          int ten = jiaoyanhe & (0xff00);
          int eleven = jiaoyanhe & (0xff);
          output[10] = ten >> 8; //首部校验和
          output[11] = eleven;   //首部校验和
          // ...
          // RIP
          // checksum calculation for ip and udp
          // if you don't want to calculate udp checksum, set it to zero
          // send it back
          //std::cout << "现在是收到了一个request，把request想要的路由表发过去" << std::endl;
          HAL_SendIPPacket(if_index, output, rip_len + 20 + 8, src_mac);
        }
        else
        {
          //rip是一个response
          //所以要根据这个rip更新路由表
          // response
          // TODO: use query and update
          //std::cout << "rip is response " << rip.numEntries << std::endl;
          RoutingTableEntry table_response[rip.numEntries];
          for (int i = 0; i < rip.numEntries; i++) //把rip.entries 构造成 routingtableEntry 结构体
          {
            table_response[i].addr = rip.entries[i].addr;
            table_response[i].len = mask_to_len(rip.entries[i].mask); //小端序的长度
            table_response[i].nexthop = src_addr;
            table_response[i].metric = swap_endian(swap_endian(rip.entries[i].metric) + 1); //到目标的距离加一  大端变成小端加一，再变成大端
            table_response[i].if_index = if_index;
            table_response[i].timestamp = HAL_GetTicks();
          }
          //先查询：
          bool route_change_flag = false;
          bool find_flag = false;
          for (int i = 0; i < rip.numEntries; i++)
          {
            uint32_t if_index_temp;
            uint32_t nexthop_temp;
            find_flag = false;
            for (int j = 0; j < length_table; j++)
            {
              if (table_response[i].addr == table[j].addr && table_response[i].len == table[j].len)
              { //查询到本地有对应的表项
                find_flag = true;
                if (table[j].nexthop == src_addr) //如果来自同一个邻居 而且 下一跳相同
                {
                  //TODO 初始化计时器
                  if (table[j].metric != table_response[i].metric) //下一跳相同时，只要metric不相等就更新
                  {
                    //std::cout << "update1" << std::endl;
                    update(swap_endian(table_response[i].metric) != 16, table_response[i]); //如果metric==16 那么直接删除，反之 添加
                    route_change_flag = true;
                  }
                }
                else if (swap_endian(table[j].metric) >= swap_endian(table_response[i].metric)) //若来自不同的邻居，比较metric大小进行选择 ,若新metric等于旧metric，最简单的就是什么都不做
                {
                  //std::cout << "update2" << std::endl;
                  update(true, table_response[i]);
                  route_change_flag = true;
                }
              }
            }
            if (!find_flag) //没有找到也更新 （进行表项的添加）
            {
              //std::cout << "update3" << std::endl;
              update(true, table_response[i]);   //所以更新之后我的本地路由表metric也是大端
              route_change_flag = true;
            }
          }
          //组播出去
          if (route_change_flag) //只要有改变，那么打包发送我的整个路由表
            dabaofasong();
        }
      }
    }
    else
    {
      // forward
      // beware of endianness
      //std::cout << "开始转发" << std::endl;
      uint32_t nexthop, dest_if;
      if (query(dst_addr, &nexthop, &dest_if))
      {
        // found
        macaddr_t dest_mac;
        // direct routing
        if (nexthop == 0)
        {
          nexthop = dst_addr;
        }
        if (HAL_ArpGetMacAddress(dest_if, nexthop, dest_mac) == 0)
        {
          // found
          memcpy(output, packet, res);
          // update ttl and checksum
          forward(output, res);
          // TODO: you might want to check ttl=0 case
          //std::cout << "转发" << dest_if << ' ' << nexthop << ' ' << res << std::endl;
          HAL_SendIPPacket(dest_if, output, res, dest_mac);
        }
        else
        {
          //std::cout << "mac not found" << std::endl;
          // not found
        }
      }
      else
      {
        //std::cout << "路由表not found" << std::endl;
        // not found
      }
    }
  }
  return 0;
}
