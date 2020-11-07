#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define TAM_LINHA 30 // tamanho máximo dos comandos

int main(){
    char **comando, in[100], *token;
    int cmd_div[30]; // vetor com a posição de cada pipe
    int i,j; // contador
    int tam; // tamanho da matriz de comandos usado para impressão da mesma
    pid_t child; // processo filho

    scanf("%[^\n]s",in); // scanf() guarda a linha toda na string in
    token = strtok(in," "); // tokeniza a entrada usando espaço como separador
    comando = (char**) calloc(TAM_LINHA*sizeof(char*), 1); //matriz de comando

    // insere cada palavra em uma linha da matriz de comandos
    i = 0; // contador de token
    j = 1; // contador de blocos de comando
    cmd_div[0]=0;
    while(token != NULL){
        comando[i] = (char*) calloc(strlen(token)*sizeof(char*), 1);
        // parser para encontar os pipes
        if(strcmp(token,"|") == 0){
            cmd_div[j] = i+1;
            comando[i] = NULL; // o pipe precisa ser null para o execvp() saber onde parar de executar o comando
            j++;
        }else{
            strcpy(comando[i],token); // copia palavra do comando para a matriz
        }
        i++; //próximo token
        tam++; //incrementa o tamanho
        token = strtok(NULL," "); // pega o próximo token da string 'in'
    }

    // alocação do descritor de arquivos
    int fd[j][2]; // descritor de arquivos

    for(i=0;i<j;i++){
        pipe(fd[i]); // cria um pipe para cada descritor de arquivos
    }

    for(i=0;i<j;i++){ // j guarda a quantiade de forks necessários
        child = fork();
        if(child == 0){ // processo filho 
            int FILE_out, FILE_in; // arquivos para redirecionamento de entrada ou saida
            int indice = cmd_div[i]; // indice do comando
            int point = indice; // aponta para o elemento do comando analisado

            while(comando[point] != NULL){
                if(strcmp(comando[point],">")==0){ // redirecionamento de saída
                    FILE_out = open(comando[point+1], O_CREAT | O_RDWR | O_TRUNC, 0644);
                    dup2(FILE_out,STDOUT_FILENO);
                    comando[point] = NULL; // indicação para execvp() parar a leitura

                }else if(strcmp(comando[point],">>")==0){ // apêncide de saída
                    FILE_out = open(comando[point+1], O_CREAT | O_RDWR | O_APPEND, 0644);
                    dup2(FILE_out,STDOUT_FILENO);
                    comando[point] = NULL; // indicação para execvp() parar a leitura

                }else if(strcmp(comando[point],"<")==0){ // redirecionamento de entrada
                    FILE_in = open(comando[point+1], O_RDONLY, 0644);
                    dup2(FILE_in,STDIN_FILENO);
                    comando[point] = NULL; // indicação para execvp() parar a leitura
                }
                point++;
            }

            if(i != 0){ // caso nao seja o primeiro processo
                close(fd[i-1][1]); // fecha o descritor de escrita
                dup2(fd[i-1][0], STDIN_FILENO); // muda o descritor de leitura para stdin
				close(fd[i-1][0]); // fecha o descritor de leitura
            }
            if(i != j-1){ // caso não seja o ultimo processo
                close(fd[i][0]); // fecha o descritor de leitura
				dup2(fd[i][1], STDOUT_FILENO); // muda o descritor de escrita para stdout
				close(fd[i][1]); // fecha o descritor de escrita
            }
            execvp(comando[indice],&comando[indice]);
			close(fd[i-1][0]);

        }else if(child>0){ // processo pai
			close(fd[i-1][0]);
			close(fd[i-1][1]);
			waitpid(-1, NULL, 0); // pai aguarda os filhos finalizarem
		}else{
            printf("Erro ao criar processos filhos!!\n");
        }
    }
    return 0;
}
