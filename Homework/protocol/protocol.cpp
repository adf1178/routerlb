#include "rip.h"
#include <stdint.h>
#include <stdlib.h>

/*
  在头文件 rip.h 中定义了如下的结构体：
  #define RIP_MAX_ENTRY 25
  typedef struct {
    // all fields are big endian
    // we don't store 'family', as it is always 2(response) and 0(request)
    // we don't store 'tag', as it is always 0
    uint32_t addr;
    uint32_t mask;
    uint32_t nexthop;
    uint32_t metric;
  } RipEntry;

  typedef struct {
    uint32_t numEntries;
    // all fields below are big endian
    uint8_t command; // 1 for request, 2 for response, otherwsie invalid
    // we don't store 'version', as it is always 2
    // we don't store 'zero', as it is always 0
    RipEntry entries[RIP_MAX_ENTRY];
  } RipPacket;

  你需要从 IPv4 包中解析出 RipPacket 结构体，也要从 RipPacket 结构体构造出对应的 IP 包
  由于 Rip 包结构本身不记录表项的个数，需要从 IP 头的长度中推断，所以在 RipPacket 中额外记录了个数。
  需要注意这里的地址都是用 **大端序** 存储的，1.2.3.4 对应 0x04030201 。
*/

/**
 * @brief 从接受到的 IP 包解析出 Rip 协议的数据
 * @param packet 接受到的 IP 包
 * @param len 即 packet 的长度
 * @param output 把解析结果写入 *output
 * @return 如果输入是一个合法的 RIP 包，把它的内容写入 RipPacket 并且返回 true；否则返回 false
 *
 * IP 包的 Total Length 长度可能和 len 不同，当 Total Length 大于 len 时，把传入的 IP 包视为不合法。
 * 你不需要校验 IP 头和 UDP 的校验和是否合法。
 * 你需要检查 Command 是否为 1 或 2，Version 是否为 2， Zero 是否为 0，
 * Family 和 Command 是否有正确的对应关系，Tag 是否为 0，
 * Metric 转换成小端序后是否在 [1,16] 的区间内，
 * Mask 的二进制是不是连续的 1 与连续的 0 组成等等。
 */
bool exam10(uint32_t mask)
{
  uint32_t yu = 0x80000000;
  while ((mask & yu))
  {
    mask = mask << 1;
    //yu = yu >>1;
  }
  if (mask > 0)
    return false;
  else
    return true;
}
bool disassemble(const uint8_t *packet, uint32_t len, RipPacket *output)
{
  // TODO:

  uint32_t offset = 4 * (packet[0] & (0xf))+8;
  uint32_t command = packet[0 + offset];
  uint32_t version = packet[1 + offset];
  uint32_t Zero = ((uint32_t)packet[2 + offset] << 8) + packet[3 + offset];
  uint32_t tag = ((uint32_t)packet[6 + offset] << 8) + packet[7 + offset];
  if ((command != 1 && command != 2) || version != 2 || Zero != 0 || tag != 0)
    return false;
  uint32_t total_length = ((uint32_t)packet[2] << 8) + packet[3];
  if (total_length > len)
    return false;
  uint32_t family = ((uint32_t)packet[4 + offset] << 8) + packet[5 + offset];
  if ((command == 1 && family != 0) || (command == 2 && family != 2))
    return false;

  uint32_t numentry = (total_length - offset - 4) / 20;
  for (int j = 0; j < numentry; j++)
  {
    //这里把mask转换成前面是1后面是0的形式
    //注意packet里面mask大端，metric小端
    uint32_t metric_small_end = ((uint32_t)packet[20 + offset+j*20] << 24) + ((uint32_t)packet[21 + offset+j*20] << 16) + ((uint32_t)packet[22 + offset+j*20] << 8) + ((uint32_t)packet[23 + offset]);
    if (metric_small_end > 16 || metric_small_end < 1)
      return false;
    uint32_t mask_small_end = ((uint32_t)packet[12 + offset+j*20] << 24) + ((uint32_t)packet[13 + offset+j*20] << 16) + ((uint32_t)packet[14 + offset+j*20] << 8) + ((uint32_t)packet[15 + offset]);
    if (!exam10(mask_small_end))
      return false;
  }
  output->command = command;
  output->numEntries = numentry;
  for (int i = 0; i < output->numEntries; i++)
  {
    //传出去的时候全部都是大端
    output->entries[i].addr = ((uint32_t)packet[11 + offset + i * 20] << 24) + ((uint32_t)packet[10 + offset + i * 20] << 16) + ((uint32_t)packet[9 + offset + i * 20] << 8) + ((uint32_t)packet[8 + offset + i * 20]);//大端序
    output->entries[i].mask = ((uint32_t)packet[15 + offset + i * 20] << 24) + ((uint32_t)packet[14 + offset + i * 20] << 16) + ((uint32_t)packet[13 + offset + i * 20] << 8) + ((uint32_t)packet[12 + offset + i * 20]);//大端序
    output->entries[i].nexthop = ((uint32_t)packet[19 + offset + i * 20] << 24) + ((uint32_t)packet[18 + offset + i * 20] << 16) + ((uint32_t)packet[17 + offset + i * 20] << 8) + ((uint32_t)packet[16 + offset + i * 20]);//大端序
    output->entries[i].metric = ((uint32_t)packet[23 + offset + i * 20] << 24) + ((uint32_t)packet[22 + offset + i * 20] << 16) + ((uint32_t)packet[21 + offset + i * 20] << 8) + ((uint32_t)packet[20 + offset + i * 20]);//大端序
  }
  return true;
}

/**
 * @brief 从 RipPacket 的数据结构构造出 RIP 协议的二进制格式
 * @param rip 一个 RipPacket 结构体
 * @param buffer 一个足够大的缓冲区，你要把 RIP 协议的数据写进去
 * @return 写入 buffer 的数据长度
 *
 * 在构造二进制格式的时候，你需要把 RipPacket 中没有保存的一些固定值补充上，包括 Version、Zero、Address Family 和 Route Tag 这四个字段
 * 你写入 buffer 的数据长度和返回值都应该是四个字节的 RIP 头，加上每项 20 字节。
 * 需要注意一些没有保存在 RipPacket 结构体内的数据的填写。
 */
uint32_t assemble(const RipPacket *rip, uint8_t *buffer)
{
  // TODO:
  buffer[0] = rip->command;//command
  buffer[1] = 2;//version
  buffer[2] = 0;//zero
  buffer[3] = 0;//zero

  for (int i = 0; i < rip->numEntries; i++)
  {
    buffer[4 + i * 20] = 0;
    if (rip->command == 1)//family
        buffer[5+i*20] = 0;
    else if(rip->command == 2)
        buffer[5+i*20] = 2;
    buffer[6+i*20] = 0;//tag
    buffer[7+i*20] = 0;//tag
    //ip address
    buffer[8+i*20] = rip->entries[i].addr & (0xff);
    buffer[9+i*20] = (rip->entries[i].addr >> 8)& (0xff);
    buffer[10+i*20] = (rip->entries[i].addr >> 16)& (0xff);
    buffer[11+i*20] = (rip->entries[i].addr >> 24)& (0xff);
    //mask
    buffer[12+i*20] = rip->entries[i].mask & (0xff);
    buffer[13+i*20] = (rip->entries[i].mask >> 8)& (0xff);
    buffer[14+i*20] = (rip->entries[i].mask >> 16)& (0xff);
    buffer[15+i*20] = (rip->entries[i].mask >> 24)& (0xff);
    //next hop
    buffer[16+i*20] = rip->entries[i].nexthop & (0xff);
    buffer[17+i*20] = (rip->entries[i].nexthop >> 8)& (0xff);
    buffer[18+i*20] = (rip->entries[i].nexthop >> 16)& (0xff);
    buffer[19+i*20] = (rip->entries[i].nexthop >> 24)& (0xff);
    //metric
    buffer[20+i*20] = rip->entries[i].metric & (0xff);
    buffer[21+i*20] = (rip->entries[i].metric >> 8)& (0xff);
    buffer[22+i*20] = (rip->entries[i].metric >> 16)& (0xff);
    buffer[23+i*20] = (rip->entries[i].metric >> 24)& (0xff);
  }

  return rip->numEntries*20+4;
}
