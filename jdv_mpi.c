#include "mpi/mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAM 2048
#define NUM_GERACOES 2000

int linhas = 0;
int numGeracoes = 0;
int vivos = 0;

int **primeiraMatriz, **segundaMatriz;
int totalProcs = 0, idProcesso = 0;

void inicializa() {
    int i, j;
    primeiraMatriz = malloc((linhas + 2) * sizeof(int *));
    segundaMatriz = malloc((linhas + 2) * sizeof(int *));

    for (i = 0; i <= linhas + 1; i++) {
        primeiraMatriz[i] = malloc(TAM * sizeof(int));
        segundaMatriz[i] = malloc(TAM * sizeof(int));

        for (j = 0; j < TAM; j++) {
            primeiraMatriz[i][j] = 0;
            segundaMatriz[i][j] = 0;
        }
    }

}

void preencher(int **matriz) { // definindo o que foi solicitado no trabalho
    //GLIDER
    int lin = 1, col = 1;
    matriz[lin][col + 1] = 1;
    matriz[lin + 1][col + 2] = 1;
    matriz[lin + 2][col] = 1;
    matriz[lin + 2][col + 1] = 1;
    matriz[lin + 2][col + 2] = 1;

    //R-pentomino
    lin = 10;
    col = 30;
    matriz[lin][col + 1] = 1;
    matriz[lin][col + 2] = 1;
    matriz[lin + 1][col] = 1;
    matriz[lin + 1][col + 1] = 1;
    matriz[lin + 2][col + 1] = 1;
}

void trocarDados(int **Tabuleiro) {
    MPI_Request request;
    int ant, prox;

    if (idProcesso == 0)
        ant = totalProcs - 1;
    else
        ant = idProcesso - 1;

    prox = (idProcesso + 1) % totalProcs;

    MPI_Isend(Tabuleiro[1], TAM, MPI_INT, ant, 0, MPI_COMM_WORLD, &request);
    MPI_Isend(Tabuleiro[linhas], TAM, MPI_INT, prox, 0, MPI_COMM_WORLD, &request);
    MPI_Recv(Tabuleiro[linhas + 1], TAM, MPI_INT, prox, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(Tabuleiro[0], TAM, MPI_INT, ant, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

}

void copia(int **Tabuleiro) {
    int i;
    for (i = 0; i < TAM; i++) {
        Tabuleiro[0][i] = Tabuleiro[linhas][i];
        Tabuleiro[linhas + 1][i] = Tabuleiro[1][i];
    }
}

int getNeighbors(int i, int j, int **matriz) {
    int linha_cima, linha_meio, linha_baixo;
    int coluna_esquerda, coluna_meio, coluna_direita;
    int vivos = 0;

    //linhas
    linha_meio = i;

    if (i == 0) // se a linha atual for a linha 0, a linha de cima é a TAM - 1
        linha_cima = TAM - 1;
    else // caso contrário, a linha de cima é i - 1
        linha_cima = i - 1;

    linha_baixo = (i + 1) %
                  TAM; // se i+1 for inferior a TAM, o resto da divisão (%) é i+1; se i+1=TAM, o resto é 0, ou seja, volta para a linha 0;

    // colunas
    coluna_meio = j;

    if (j == 0) // se a coluna atual for a 0, a coluna da esquerda é a TAM-1
        coluna_esquerda = TAM - 1;
    else // caso contrário, a coluna da esquerda é j - 1
        coluna_esquerda = j - 1;

    coluna_direita = (j + 1) %
                     TAM; // se j+1 for inferior a TAM, o resto da divisão (%) é j+1; se j+1=TAM, o resto é 0, ou seja, volta para a coluna 0;

    // verificando os vizinhos vivos
    vivos = vivos + matriz[linha_cima][coluna_esquerda];
    vivos = vivos + matriz[linha_meio][coluna_esquerda];
    vivos = vivos + matriz[linha_baixo][coluna_esquerda];
    vivos = vivos + matriz[linha_cima][coluna_meio];
    vivos = vivos + matriz[linha_baixo][coluna_meio];
    vivos = vivos + matriz[linha_cima][coluna_direita];
    vivos = vivos + matriz[linha_meio][coluna_direita];
    vivos = vivos + matriz[linha_baixo][coluna_direita];

    return vivos;
}

void geracoes(int **geracaoAtual, int **novaGeracao) { // criando uma nova geração
    int i, j, vizinhosVivos;

    if (totalProcs > 1)
        trocarDados(geracaoAtual);
    else
        copia(geracaoAtual);

    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 1; i <= linhas; i++) {
        for (j = 0; j < TAM; j++) {
            // Verificando os vizinhos vivos da célula na posição i, j
            vizinhosVivos = getNeighbors(i, j, geracaoAtual);

            // Qualquer célula viva com 2 (dois) ou 3 (três) vizinhos deve sobreviver;
            if (geracaoAtual[i][j] == 1 && (vizinhosVivos == 2 || vizinhosVivos == 3))
                novaGeracao[i][j] = 1;

                // Qualquer célula morta com 3 (três) vizinhos torna-se viva;
            else if (geracaoAtual[i][j] == 0 && vizinhosVivos == 3)
                novaGeracao[i][j] = 1;

                // Qualquer outro caso, células vivas devem morrer e células já mortas devem continuar mortas.
            else
                novaGeracao[i][j] = 0;
        }
    }
    numGeracoes += 1;
}

int totalVivos(int **matriz) { // verificando a quantidade de células vivas em uma matriz
    int i, j, var, vivos = 0;

    i = (linhas / totalProcs) * idProcesso;
    if (idProcesso < totalProcs - 1) {
        var = (linhas / totalProcs) * idProcesso;
    } else {
        var = linhas;
    }
    for (; i <= var; i++) {
        for (j = 0; j < TAM; j++)
            vivos = vivos + matriz[i][j];
    }
    return vivos;
}

void jogoDaVida() {
    int i;
    for (i = 1; i <= NUM_GERACOES; i++) {
        if (i % 2 != 0) //se i é impar
            geracoes(primeiraMatriz, segundaMatriz);
        else  //se i é par
            geracoes(segundaMatriz, primeiraMatriz);
    }
}

int main(int argc, char *argv[]) {
    time_t start, end;
    start = time(NULL);

    MPI_Init(&argc, &argv); //Inicializa a execução MPI

    MPI_Comm_size(MPI_COMM_WORLD, &totalProcs); //Define numero de processos
    MPI_Comm_rank(MPI_COMM_WORLD, &idProcesso); //Atribui ID do processo

    if (idProcesso == 1) {
        printf("Jogo da vida\n");
    }
    linhas = (TAM / totalProcs);
    if (idProcesso == totalProcs - 1) {
        linhas += TAM % totalProcs;
    }

    inicializa();
 
    preencher(primeiraMatriz);

    MPI_Barrier(MPI_COMM_WORLD);

    jogoDaVida(); // executando o jogo

    MPI_Barrier(MPI_COMM_WORLD);

    if (NUM_GERACOES % 2 == 0) {
        vivos += totalVivos(primeiraMatriz);
    } else {
        vivos += totalVivos(segundaMatriz);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (idProcesso == 0) {
        printf("final = %d\n", vivos);
    }

    MPI_Finalize();

    end = time(NULL);
    printf("Tempo: %ld\n", end - start);

    return 0;
}
