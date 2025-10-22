#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <math.h>

#define TX_POWER -59  // TX Power (기본값 -59 dBm)

// RSSI 값을 기반으로 거리를 계산하는 함수
double calculate_distance(int rssi) {
    double N = 2.0;  // 환경에 따라 2~4 사이의 값
    return pow(10, (TX_POWER - rssi) / (10 * N));
}

// 주변 Bluetooth 장치 스캔 함수
int scan_for_devices() {
    int dev_id, sock;
    struct hci_dev_info di;
    struct hci_request rq;
    uint8_t buffer[HCI_MAX_EVENT_SIZE];
    int num_rsp;
    inquiry_info *ii = NULL;

    dev_id = hci_get_route(NULL);
    if (dev_id < 0) {
        perror("Bluetooth 장치 찾기 실패");
        return -1;
    }

    sock = hci_open_dev(dev_id);
    if (sock < 0) {
        perror("소켓 열기 실패");
        return -1;
    }

    // 주변 장치 검색 시작 (일반적으로 8초 동안 검색)
    ii = (inquiry_info*)malloc(255 * sizeof(inquiry_info));
    num_rsp = hci_inquiry(dev_id, 8, 255, NULL, ii, IREQ_CACHE_FLUSH);

    if (num_rsp < 0) {
        perror("장치 검색 실패");
        free(ii);
        close(sock);
        return -1;
    }

    printf("주변 장치들:\n");
    for (int i = 0; i < num_rsp; i++) {
        char addr[19] = { 0 };
        ba2str(&ii[i].bdaddr, addr);
        printf("%s\n", addr);
    }

    free(ii);
    close(sock);
    return 0;
}

// 특정 Bluetooth 장치에 대해 RSSI 값을 읽는 함수
int get_rssi(const char *device_address) {
    int dev_id, sock, rssi;
    struct hci_dev_info di;
    bdaddr_t bdaddr;
    struct hci_request rq;
    uint8_t buffer[HCI_MAX_EVENT_SIZE];

    // 장치 주소를 bdaddr_t 구조체로 변환
    str2ba(device_address, &bdaddr);

    // 장치 ID 얻기
    dev_id = hci_get_route(NULL);
    if (dev_id < 0) {
        perror("Bluetooth 장치 찾기 실패");
        return -1;
    }

    // Bluetooth 장치 정보 가져오기
    sock = hci_open_dev(dev_id);
    if (sock < 0) {
        perror("소켓 열기 실패");
        return -1;
    }

    // 주변 장치의 RSSI 값을 읽기 위한 요청
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_READ_RSSI;
    rq.cparam = (void *)&bdaddr;
    rq.clen = sizeof(bdaddr);

    // 요청 전송
    if (hci_send_req(sock, &rq, 1000) < 0) {
        perror("RSSI 읽기 실패");
        close(sock);
        return -1;
    }

    // RSSI 값이 buffer에 저장될 것으로 예상, buffer[0]을 통해 RSSI 값 읽기
    rssi = buffer[0];  // 실제 응답에서 적절히 수정 필요

    close(sock);
    return rssi;
}

int main() {
    char device_address[18];
    int rssi;

    // 주변 장치 검색 및 출력
    printf("Bluetooth 주변 장치 검색 중...\n");
    if (scan_for_devices() != 0) {
        printf("장치 검색 중 오류가 발생했습니다.\n");
        return -1;
    }

    // 사용자로부터 특정 BD 주소 입력 받기
    printf("RSSI 값을 읽고 싶은 BD Address를 입력하세요 (형식: XX:XX:XX:XX:XX:XX): ");
    scanf("%17s", device_address);

    // 입력된 주소에서 RSSI 값을 읽어오기
    rssi = get_rssi(device_address);
    
    if (rssi != -1) {
        printf("RSSI 값: %d dBm\n", rssi);
        
        // RSSI 값을 기반으로 거리 계산
        double distance = calculate_distance(rssi);
        printf("대략적인 거리: %.2f 미터\n", distance);
    } else {
        printf("RSSI 값을 읽을 수 없습니다.\n");
    }

    return 0;
}

