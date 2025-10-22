#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

typedef struct {
    uint32_t status;
    uint32_t ctrl; 
} GPIOregs;

typedef struct {
    uint32_t Out;
    uint32_t OE;
    uint32_t In;
    uint32_t InSync;
} rioregs;

#define BCM_IO_BASE 0xFE000000 /* Raspberry Pi 4/5의 I/O Peripherals 주소 */
#define GPIO_BASE (BCM_IO_BASE + 0x200000) /* GPIO 컨트롤러의 주소 */
#define RIO_BASE (BCM_IO_BASE + 0xE0000) /* RIO 컨트롤러의 주소 */
#define PAD_BASE (BCM_IO_BASE + 0xF0000) /* PAD 컨트롤러의 주소 */

volatile GPIOregs *GPIO; /* GPIO 레지스터 포인터 */
volatile rioregs *rioSET; /* RIO 설정 레지스터 포인터 */
volatile rioregs *rioXOR; /* RIO XOR 레지스터 포인터 */

int main(int argc, char **argv) {
    int memfd = open("/dev/mem", O_RDWR | O_SYNC);
    if (memfd < 0) {
        perror("open() /dev/mem");
        return -1;
    }

    uint32_t *map = (uint32_t *)mmap(NULL, 64 * 1024 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0x1F00000000);
    if (map == MAP_FAILED) {
        printf("mmap failed: %s\n", strerror(errno));
        close(memfd);
        return -1;
    }
    close(memfd);

    GPIO = (GPIOregs *)(map + (GPIO_BASE - BCM_IO_BASE) / sizeof(uint32_t));
    rioSET = (rioregs *)(map + (RIO_BASE - BCM_IO_BASE) / sizeof(uint32_t));
    rioXOR = (rioregs *)(map + (RIO_BASE - BCM_IO_BASE + 0x1000) / sizeof(uint32_t));

    uint32_t pin = 18; // 사용할 GPIO 핀 번호
    uint32_t fn = 5; // GPIO 기능 설정 (출력)

    // GPIO 설정
    GPIO[pin].ctrl = fn; 
    *((uint32_t *)((uint8_t *)map + (PAD_BASE - BCM_IO_BASE))) = 0x10; // PAD 설정
    rioSET->OE = 0x01 << pin; // 핀을 출력으로 설정
    rioSET->Out = 0x01 << pin; // 핀을 HIGH로 설정

    for (;;) {
        sleep(1);
        rioXOR->Out = 0x01 << pin; // 핀을 토글
        sleep(1);
    } 

    munmap(map, 64 * 1024 * 1024); // 메모리 해제

    return EXIT_SUCCESS;
}
