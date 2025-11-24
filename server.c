/* Projeto 2 - Sockets em C
André Vinícius Zicka Schmidt & Enzo Fabrizzio Moro Conke

Arquivo: server.c
Hub de Coleta de Dados Central - Estação Meteorológica IoT
Protocolo SMSP (Sistema de Mensagens Simplificado)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define BUFLEN 512  // Tamanho máximo do buffer
#define PORT 10001  // Porta do servidor

void die(char *s) {
    perror(s);
    exit(1);
}

// Função para processar mensagens do protocolo SMSP
// Formato: TIPO,TAMANHO,CORPO
void process_message(char *buf, char *response) {
    char tipo[4];
    char tamanho[4];
    char corpo[BUFLEN];

    // Extrai o TIPO (3 caracteres)
    strncpy(tipo, buf, 3);
    tipo[3] = '\0';

    // Extrai o TAMANHO (3 dígitos)
    strncpy(tamanho, buf + 4, 3);
    tamanho[3] = '\0';

    int body_size = atoi(tamanho);

    // Extrai o CORPO se houver
    if (body_size > 0) {
        strncpy(corpo, buf + 8, body_size);
        corpo[body_size] = '\0';
    } else {
        corpo[0] = '\0';
    }

    printf("  [TIPO]: %s\n", tipo);
    printf("  [TAMANHO]: %s (%d bytes)\n", tamanho, body_size);
    printf("  [CORPO]: %s\n", body_size > 0 ? corpo : "(vazio)");

    // Processa comando GET
    if (strncmp(tipo, "GET", 3) == 0) {
        printf("  [ACAO]: Solicitacao de informacoes recebida\n");

        // Responde com ACK e status do sistema
        char ack_body[] = "Sistema OK - Hub operacional";
        sprintf(response, "ACK,%03d,%s", (int)strlen(ack_body), ack_body);
    }
    // Processa comando SND
    else if (strncmp(tipo, "SND", 3) == 0) {
        printf("  [ACAO]: Temperatura recebida: %s\n", corpo);

        // Responde com ACK de confirmação
        char ack_body[] = "Dados recebidos com sucesso";
        sprintf(response, "ACK,%03d,%s", (int)strlen(ack_body), ack_body);
    }
    // Comando inválido
    else {
        printf("  [ACAO]: Comando invalido detectado\n");

        // Responde com ERR
        char err_body[] = "Comando Invalido";
        sprintf(response, "ERR,%03d,%s", (int)strlen(err_body), err_body);
    }
}

int main(void) {
    struct sockaddr_in si_me, si_other;
    int s, slen = sizeof(si_other), recv_len;
    char buf[BUFLEN];
    char response[BUFLEN];

    printf("=== SERVIDOR SMSP - Estacao Meteorologica IoT ===\n");
    printf("Protocolo: UDP\n");
    printf("Porta: %d\n\n", PORT);

    // Criação do Socket IPv4 UDP
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("socket");

    memset((char *) &si_me, 0, sizeof(si_me));

    // Configuração do Endereço do Servidor
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    // Binding do socket ao endereço e porta -> escutar
    if(bind(s, (struct sockaddr*)&si_me, sizeof(si_me)) == -1)
        die("bind");

    printf("Servidor iniciado. Aguardando conexoes...\n\n");

    // Loop principal de execução
    while(1) {
        printf("Esperando dados do cliente...\n");
        fflush(stdout);

        memset(buf, '\0', BUFLEN);

        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
            die("recvfrom()");

        buf[recv_len] = '\0';

        printf("\n>>> Pacote recebido de %s:%d\n",
               inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Mensagem bruta: %s\n", buf);

        // Processa a mensagem SMSP
        memset(response, '\0', BUFLEN);
        process_message(buf, response);

        printf("  [RESPOSTA]: %s\n", response);

        // Envia resposta ao cliente
        if (sendto(s, response, strlen(response), 0, (struct sockaddr*) &si_other, slen) == -1)
            die("sendto()");

        printf("<<< Resposta enviada\n\n");
    }

    close(s);
    return 0;
}