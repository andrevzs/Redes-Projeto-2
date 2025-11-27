/* Projeto 2 - Sockets em C
André Vinícius Zicka Schmidt & Enzo Fabrizzio Moro Conke

Arquivo: client.c
Microcontrolador de baixo consumo de energia - Estação Meteorológica IoT
Ocasionalmente desperta (wake up) para reportar dados ou verificar estado do sistema.
Protocolo SMSP (Sistema de Mensagens Simplificado)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define SERVER "127.0.0.1"  // Loopback
#define BUFLEN 512  // Tamanho máximo do buffer
#define PORT 10001  // Porta do servidor

// Função de erro, imprime mensagem de erro e encerra o programa
void die(char *s) {
    perror(s);
    exit(1);
}

// Função para gerar temperatura aleatória
int generate_temperature() {
    return (rand() % 31) + 10;  // Temperatura entre 10°C e 40°C
}

// Função para criar mensagem GET (solicitar informações)
void create_get_message(char *message) {
    sprintf(message, "GET,000,");
}

// Função para criar mensagem SND (enviar temperatura)
void create_snd_message(char *message, int temperature) {
    char body[BUFLEN];
    sprintf(body, "Temp: %dC", temperature);
    sprintf(message, "SND,%03d,%s", (int)strlen(body), body);
}

// Função para processar resposta do servidor
void process_response(char *buf, int last_command_was_get) {
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

    if (strncmp(tipo, "ACK", 3) == 0) {
        printf("  [STATUS]: Confirmacao recebida\n");

        // Se foi resposta ao GET, exibe a hora na tela
        if (last_command_was_get && body_size > 0) {
            printf("\n*** HORA RECEBIDA DO SERVIDOR ***\n");
            printf("*** %s ***\n\n", corpo);
        }
        // Se foi resposta ao SND, destaca o OK
        else if (!last_command_was_get && body_size > 0) {
            printf("\n*** CONFIRMACAO: %s ***\n\n", corpo);
        }
    } else if (strncmp(tipo, "ERR", 3) == 0) {
        printf("  [STATUS]: Erro reportado pelo servidor\n");
    }
}

int main(void) {
    struct sockaddr_in si_other;
    int s, slen=sizeof(si_other);
    char buf[BUFLEN];
    char message[BUFLEN];
    int opcao;

    // Inicializa gerador de números aleatórios
    srand(time(NULL));

    printf("=== CLIENTE SMSP - Estacao Meteorologica IoT ===\n");
    printf("Servidor: %s:%d\n", SERVER, PORT);
    printf("Protocolo: UDP\n\n");

    // Criação do Socket IPv4 UDP
    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("socket");

    // Configuração do Endereço do Servidor
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    if (inet_aton(SERVER, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() falhou!\n");
        exit(1);
    }

    printf("Conectado ao servidor. Pronto para enviar dados.\n\n");

    // Loop Principal de execução
    while(1) {
        printf("\n=== Menu de Opcoes ===\n");
        printf("1 - Enviar comando GET (solicitar informacoes)\n");
        printf("2 - Enviar comando SND (enviar temperatura)\n");
        printf("3 - Executar fluxo completo automatico (GET -> SND -> Fechar)\n");
        printf("4 - Enviar comando INVALIDO (testar resposta ERR)\n");
        printf("0 - Sair\n");
        printf("Escolha uma opcao: ");

        scanf("%d", &opcao);
        getchar(); // Limpa o buffer do teclado

        if (opcao == 0) {
            printf("Encerrando cliente...\n");
            break;
        }

        // Opção 3: Fluxo automático conforme protocolo da imagem
        if (opcao == 3) {
            printf("\n========================================\n");
            printf("EXECUTANDO FLUXO AUTOMATICO DO PROTOCOLO\n");
            printf("========================================\n");

            // Passo 1: Enviar GET
            printf("\n[PASSO 1] Enviando comando GET (solicitar hora atual)...\n");
            memset(message, '\0', BUFLEN);
            create_get_message(message);
            printf("Mensagem formatada: %s\n", message);

            if (sendto(s, message, strlen(message), 0, (struct sockaddr *) &si_other, slen) == -1)
                die("sendto()");
            printf("<<< Mensagem GET enviada\n");

            // Passo 2: Receber hora atual
            memset(buf, '\0', BUFLEN);
            printf("\n[PASSO 2] Aguardando hora atual do servidor...\n");

            if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
                die("recvfrom()");

            printf(">>> Resposta recebida do servidor\n");
            printf("Mensagem bruta: %s\n", buf);
            process_response(buf, 1);

            // Passo 3: Enviar SND com temperatura
            printf("\n[PASSO 3] Enviando temperatura para o servidor...\n");
            memset(message, '\0', BUFLEN);
            int temperatura = generate_temperature();
            create_snd_message(message, temperatura);
            printf("Temperatura gerada: %dC\n", temperatura);
            printf("Mensagem formatada: %s\n", message);

            if (sendto(s, message, strlen(message), 0, (struct sockaddr *) &si_other, slen) == -1)
                die("sendto()");
            printf("<<< Mensagem SND enviada\n");

            // Passo 4: Receber OK
            memset(buf, '\0', BUFLEN);
            printf("\n[PASSO 4] Aguardando confirmacao do servidor...\n");

            if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
                die("recvfrom()");

            printf(">>> Resposta recebida do servidor\n");
            printf("Mensagem bruta: %s\n", buf);
            process_response(buf, 0);

            // Passo 5: Fechar conexão
            printf("\n[PASSO 5] Fechando conexao...\n");
            printf("========================================\n");
            printf("FLUXO AUTOMATICO CONCLUIDO COM SUCESSO!\n");
            printf("========================================\n");
            break;
        }

        memset(message, '\0', BUFLEN);

        // Cria a mensagem conforme o protocolo SMSP
        int is_get_command = 0;
        if (opcao == 1) {
            create_get_message(message);
            printf("\n>>> Enviando comando GET\n");
            is_get_command = 1;
        } else if (opcao == 2) {
            int temp = generate_temperature();
            create_snd_message(message, temp);
            printf("\n>>> Enviando comando SND (Temperatura: %dC)\n", temp);
        } else if (opcao == 4) {
            // Envia mensagem inválida para testar resposta ERR
            sprintf(message, "XXX,010,Teste Erro");
            printf("\n>>> Enviando comando INVALIDO (XXX)\n");
            printf(">>> Esperando resposta ERR do servidor...\n");
        } else {
            printf("Opcao invalida!\n");
            continue;
        }

        printf("Mensagem formatada: %s\n", message);

        // Envia mensagem ao servidor
        if (sendto(s, message, strlen(message), 0, (struct sockaddr *) &si_other, slen) == -1)
            die("sendto()");

        printf("<<< Mensagem enviada com sucesso\n");

        // Recebe resposta do servidor
        memset(buf, '\0', BUFLEN);

        printf("\nAguardando resposta do servidor...\n");

        if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
            die("recvfrom()");

        printf("\n>>> Resposta recebida do servidor\n");
        printf("Mensagem bruta: %s\n", buf);

        // Processa a resposta
        process_response(buf, is_get_command);
    }

    close(s);
    return 0;
}
