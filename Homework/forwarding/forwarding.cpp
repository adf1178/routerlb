#include <stdint.h>
#include <stdlib.h>
#include <iostream>
bool validateIPChecksum(uint8_t *packet, size_t len) ;
// bool validateIPChecksum(uint8_t *packet, size_t len) {
//         // TODO:
//         int sum=0;
//         int head_length = packet[0]&(0xf);
//         for (int i=0;i<4*head_length;i+=2)
//         {
//                 long temp=0,temp2=(long)packet[i];
                
//                 temp+=temp2*256;
//                 sum+=temp+packet[i+1];
//         }
//         int high_16 = (sum>>16);
//         while(high_16)
//         {
//                 sum = sum&(0xffff);
//                 sum += high_16;
//                 high_16 = (sum>>16);
//         }
//         sum =(~sum)&0xffff;
//         if (!sum)
//                 return true;
//         else return false;
// }

/**
 * @brief 进行转发时所需的 IP 头的更新：
 *        你需要先检查 IP 头校验和的正确性，如果不正确，直接返回 false ；
 *        如果正确，请更新 TTL 和 IP 头校验和，并返回 true 。
 *        你可以从 checksum 题中复制代码到这里使用。
 * @param packet 收到的 IP 包，既是输入也是输出，原地更改
 * @param len 即 packet 的长度，单位为字节
 * @return 校验和无误则返回 true ，有误则返回 false
 */

bool forward(uint8_t *packet, size_t len) {
  // TODO:
  if (validateIPChecksum(packet,len))
    {
      packet[8]-=1;
      packet[10]=0;
      packet[11]=0;
      uint32_t sum=0;
        int head_length = packet[0]&(0xf);
        for (int i=0;i<4*head_length;i+=2)
        {
                long temp=0,temp2=(long)packet[i];
                
                temp+=temp2*256;
                sum+=temp+packet[i+1];
        }
        int high_16 = (sum>>16);
        while(high_16)
        {
                sum = sum&(0xffff);
                sum += high_16;
                high_16 = (sum>>16);
        }
        sum =(~sum)&0xffff;
        int ten = sum&(0xff00);
        int eleven = sum&(0xff);
        packet[10] = ten>>8;
        packet[11] = eleven;
        return true;
            }
  else return false;
  return false;
}
