#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "web_site/html.h"
#include "web_site.h"

// Configurações de WiFi
#define WIFI_SSID "ArcherC50 2.4G"
#define WIFI_PASS "mb17012019"

// Definindo as variáveis globais
struct st_dados dados = {0}; // inicia tudo como zero
struct st_get_dados get_dados = {
    .modo = "temp",  // ou NULL, se preferir atribuir depois
    .min = {10.0,30.0,30},
    .max = {50,70,200},
    .ajuste = {0.0}
};
extern int pos_modo;  // Apenas declaração, sem atribuir valor
extern char ip_recebido[24];
// Estrutura para armazenar o estado das respostas HTTP
struct http_state
{
    char response[5300]; // Buffer para a resposta HTTP
    size_t len;          // Tamanho total da resposta
    size_t sent;         // Quantidade já enviada
};
// Callback chamado quando dados são enviados via TCP
static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    // Se toda a resposta foi enviada, fecha a conexão
    if (hs->sent >= hs->len)
    {
        tcp_close(tpcb);
        free(hs);
    }
    return ERR_OK;
}
// Callback chamado quando dados são recebidos via TCP
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }
    char *req = (char *)p->payload;
    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs)
    {
        pbuf_free(p);
        tcp_close(tpcb);
        tcp_recved(tpcb, p->tot_len);
        return ERR_MEM;
    }
    hs->sent = 0;
    // Verifica se a requisição é para obter o nível atual e estado da bomba
    if (strstr(req, "GET /dados")){
        // Cria um payload JSON com os valores atuais
        char json_payload[96];
        // printf("Dados Enviados: {\"temp\":%.2f, \"umi\":%.2f, \"pres\":%.2f}\r\n", dados.temp, dados.umi, dados.press);
        int json_len = snprintf(json_payload, sizeof(json_payload), "{\"temp\":%.2f, \"umi\":%.2f, \"pres\":%.2f}\r\n", dados.temp, dados.umi, dados.press);
       hs->len = snprintf(hs->response, sizeof(hs->response),
                   "HTTP/1.1 200 OK\r\n"
                   "Content-Type: application/json\r\n"
                   "Access-Control-Allow-Origin: *\r\n" // <-- adicionado aqui
                   "Content-Length: %d\r\n"
                   "Connection: close\r\n"
                   "\r\n"
                   "%s",
                   json_len, json_payload);

    }else if (strstr(req, "GET /setar_dados/") != NULL){
    char *pos = strstr(req, "/setar_dados/");
    if (pos){
        pos += strlen("/setar_dados/");
        char valores[64];

        char *fim = strchr(pos, ' ');
        int len = fim ? (fim - pos) : strlen(pos);
        if (len >= sizeof(valores)) len = sizeof(valores) - 1;
        strncpy(valores, pos, len);
        valores[len] = '\0';

        // printf("%s \n", valores);
        // Substitui "%7C" por "|"
        for (int i = 0; valores[i]; i++){
            if (!strncmp(&valores[i], "%7C", 3)) {
                valores[i] = '|';
                memmove(&valores[i + 1], &valores[i + 3], strlen(&valores[i + 3]) + 1);
            }
        }
        // Faz split e preenche diretamente
        char *modo = strtok(valores, "|");
        printf("%s \n",modo);
        char *min_str = strtok(NULL, "|");
        printf("%s \n",min_str);
        char *max_str = strtok(NULL, "|");
        printf("%s \n",max_str);
        char *ajuste_str = strtok(NULL, "|");
        printf("%s \n", ajuste_str);
        strcpy(get_dados.modo, modo);
        pos_modo = strcmp(modo, "temp")==0?0:strcmp(modo, "umi")==0?1:strcmp(modo, "pres")==0?2:0;
        // printf("Modo RT: %s - Posição RT: %d\n",modo,pos_modo);

        get_dados.min[pos_modo] = atof(min_str);
        get_dados.max[pos_modo] = atof(max_str);
        get_dados.ajuste[pos_modo] = atof(ajuste_str);
        // printf("dados recebidos: %s | %.2f | %.2f | %.2f\n", get_dados.modo, get_dados.min, get_dados.max, get_dados.ajuste);
        memset(valores, 0, sizeof(valores));
    }
}else{
       hs->len = snprintf(hs->response, sizeof(hs->response),
                   "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: %d\r\n"
                   "Connection: close\r\n"
                   "\r\n"
                   "%s",
                   (int)strlen(html), html);
    }
    // Configura os callbacks e envia a resposta
    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);
    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    pbuf_free(p);
    return ERR_OK;
}

// Callback chamado quando uma nova conexão é estabelecida
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

// Inicia o servidor HTTP na porta 80
static void start_http_server(void)
{
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb)
    {
        printf("Erro ao criar PCB TCP\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP rodando na porta 80...\n");
}

// Inicializa a conexão WiFi e o servidor web
void init_web_site()
{
    if (cyw43_arch_init())
    {
        printf("Erro");
        return;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Erro ao se conectar");
        return;
    }
    // Obtém e imprime o endereço IP atribuído
    uint8_t *ip = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    char ip_str[24];
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    snprintf(ip_recebido, sizeof(ip_recebido),"%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    printf("%s ", ip_str);
    start_http_server();
}