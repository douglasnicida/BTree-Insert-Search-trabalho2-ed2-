#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <math.h>

/*********************************CONSTANTES***********************************/
/* Alterar aqui, caso seja necessario */
#define ARQ_INSERE "C:\\Users\\Douglas Nicida\\Desktop\\projeto-ed2-biblioteca-arquivos-v3-main\\arquivos\\insere.bin"
#define ARQ_AB "C:\\Users\\Douglas Nicida\\Desktop\\projeto-ed2-biblioteca-arquivos-v3-main\\arquivos\\ab.bin"
#define ARQ_BUSCA "C:\\Users\\Douglas Nicida\\Desktop\\projeto-ed2-biblioteca-arquivos-v3-main\\arquivos\\busca.bin"
#define ARQ_DADOS "C:\\Users\\Douglas Nicida\\Desktop\\projeto-ed2-biblioteca-arquivos-v3-main\\arquivos\\dados.bin"

#define KEY_SIZE 14
#define MAX_KEYS 3
#define MIN_KEYS (int)floor(MAX_KEYS / 2)
#define NO_KEY '@'
#define NO 0
#define YES 1
#define NIL (-1)

/*********************************ESTRUTURAS***********************************/
typedef struct SLivro {
    char isbn[KEY_SIZE];
    char titulo[50];
    char autor[50];
    char ano[5];
} Livro;

typedef struct SABPagina {
    int ctChaves; //conta a qtd de chaves q tem na pagina no momento
    char chave[MAX_KEYS][KEY_SIZE]; // as chaves (ISBN)
    int filho[MAX_KEYS + 1]; // salva a páginas do filhos, onde cada página pode possuir no máximo 4 filhos
} ABPagina;


#define TAM_PAGINA (sizeof(int) + ((sizeof(char) * KEY_SIZE) * (sizeof(char) * MAX_KEYS)) + (sizeof(int) * (MAX_KEYS + 1)))

/********************************PROTOTIPOS************************************/

/* Sub-rotinas para manipular arvore B*/
int obterRaiz(FILE *arqAB);
int criarArvoreB(FILE *arqAB, char isbn[KEY_SIZE]);
int criarRaiz(FILE *arqAB, char isbn[KEY_SIZE], int esq, int dir);
int obterPagina(FILE *arqAB);
int criarPagina(ABPagina *novaPag);
void abEscrever(FILE *arqAB, int rrn, ABPagina *abPagina); // VER SE ISSO FUNCIONA
int inserirHeader(FILE *arqAB, int raiz);
void lerArvoreAB(FILE *arqAB, int rrn, ABPagina *abPagina);
int inserirAB(FILE *arqAB, int rrn, char chave[KEY_SIZE], int *paginaFilhoPromovido, char *chavePromovida);
int procurarNo(char chave[KEY_SIZE], ABPagina *abPagina, int *pos);
int insereNaPagina(char chave[KEY_SIZE], int rrnPromovidaBaixo, ABPagina *abPagina);
void split(FILE *arqAB, char chave[KEY_SIZE], int rrnPromovidaBaixo, ABPagina *paginaAntiga, char *chavePromovida, int *paginaFilhoPromovido, ABPagina *paginaNova);

/* Sub-rotinas doo exercicio */
FILE *abrirArquivo(char *ch);
void fecharArquivo(FILE *arq);
void obterRegistro(FILE *arq, Livro *livro);
void inserir(FILE *arqInserir, FILE *arqDados, FILE *arqAB);

/**********************************MAIN****************************************/
int main() {
    printf("TAMANHO PAGINA: %d\n", TAM_PAGINA);

    setlocale(LC_ALL, "");

    FILE *arqInserir, *arqDados, *arqAB;
    int op;

    // Menu
    printf("*-------- BIBLIOTECA --------*\n");

    do {
        printf("*______________________________________________*\n");
        printf("|  Opcao      | Funcao                         |\n");
        printf("|    1.       | Inserir                        |\n");
        printf("|    2.       | Listar todos os livros         |\n");
        printf("|    3.       | Listar livro especifico        |\n");
        printf("|    0.       | Sair do programa               |\n");
        printf("*----------------------------------------------*\n");
        printf("Digite a opcao: ");
        scanf("%d", &op);

        switch (op) {
            case 1: {
                arqInserir = NULL;
                arqDados = NULL;
                arqAB = NULL;
                inserir(arqInserir, arqDados, arqAB);
                break;
            }
            case 2: {
                printf("Listar dados de todos os livros");
                break;
            }
            case 3: {
                printf("Listar dados de um livro especifico");
                break;
            }
            case 0: {
                exit(0);
            }
            default:
                printf("Digite uma das opcoes\n");
        }
    } while (op != 0);
}

/*********************************FUNCOES**************************************/

FILE *abrirArquivo(char *ch)
{
    FILE *arq;

    if (((arq = fopen(ch, "r+b")) == NULL))
    {
        printf("ERRO: Falha ao abrir o arquivo\n%s", ch);
        return arq;
    }

    return arq;
}

void fecharArquivo(FILE *arq)
{
    fclose(arq);
}

void obterRegistro(FILE *arq, Livro *livro) {
    int ct = 1;

    if (fgetc(arq) == '@') {
        fread(&ct, sizeof(int), 1, arq);

        // Pular para posicao desejada e ler o registro
        fseek(arq, sizeof(Livro) * ct, SEEK_SET);

        fread(livro, sizeof(Livro), 1, arq);

        // Salvar o proximo registro a ser lido
        rewind(arq);
        ct++;

        fseek(arq, 1, SEEK_SET);
        fwrite(&ct, sizeof(int), 1, arq);
    } else {
        rewind(arq);

        fread(livro, sizeof(Livro), 1, arq);

        rewind(arq);
        fwrite("@", 1, sizeof(char), arq);
        fwrite(&ct, sizeof(int), 1, arq);
    }
}

void inserir(FILE *arqInserir, FILE *arqDados, FILE *arqAB) {
    Livro livro;
    char buffer[sizeof(Livro)];
    int rrn, header;
    int promovido; // indica se houve promocao para baixo
    int rrnPromovido;
    char chavePromovida[KEY_SIZE];

    arqInserir = abrirArquivo(ARQ_INSERE);
    arqDados = abrirArquivo(ARQ_DADOS);
    arqAB = abrirArquivo(ARQ_AB);

    obterRegistro(arqInserir, &livro);

    // Inserir sempre no final do arquivo de DADOS
    sprintf(buffer, "%s#%s#%s#%s#", livro.isbn, livro.titulo, livro.autor, livro.ano);
    fseek(arqDados, 0, SEEK_END);
    fwrite(buffer, 1, sizeof(Livro), arqDados);

    // Pegando RRN pagina (quantidade de registros)
    rrn = obterPagina(arqAB);

    printf("\nQuantidade de registros no arquivo: %d\n", rrn);

    // se o rrn for 0, quer dizer que deverá criar tudo do zero, se não, deve procurar um lugar para inserir
    if(rrn == 0){
        header = criarArvoreB(arqAB, livro.isbn);
    } else {
        //Atualizando novo header
        header = inserirHeader(arqAB, rrn);
        //header = obterRaiz(arqAB);

        printf("Valor header atualizado: %d \n", header);

        // verifica se houve promoção e onde será inserido
        promovido = inserirAB(arqAB, header, livro.isbn, &rrnPromovido, &chavePromovida);

        // se houver promoção, cria-se uma nova raíz
        if(promovido)
            header = criarRaiz(arqAB, chavePromovida, header, rrnPromovido);
    }

    printf("Header: %d \n", header);

    fecharArquivo(arqInserir);
    fecharArquivo(arqDados);
    fecharArquivo(arqAB);
}

int obterPagina(FILE *arqAB) {
    // Pegando a quantidade de bits no arquivo (de acordo com a quantidade de registros inserido)
    int totalRegistros, rrn;

    fseek(arqAB, 0, SEEK_END);
    totalRegistros = ftell(arqAB) - sizeof(int);

    // se não houver registros ainda
    if(totalRegistros <= 0)
        return 0;
    else 
        rrn = totalRegistros / TAM_PAGINA;
        return rrn;
}

int obterRaiz(FILE *arqAB) {
    int raiz;

    fseek(arqAB, 0, SEEK_SET);
    fread(&raiz, 1, sizeof(int), arqAB);

    if(raiz == 0) {
        printf("ERRO: Nao foi possivel obter a raiz\n");
        exit(0);
    }

    return raiz;
}

void abEscrever(FILE *arqAB, int rrn, ABPagina *abPagina) {
    int posEscrever;

    posEscrever = (rrn * TAM_PAGINA) + sizeof(int);
    fseek(arqAB, posEscrever, SEEK_SET);
    fwrite(abPagina, TAM_PAGINA, 1, arqAB);
}

void lerArvoreAB(FILE *arqAB, int rrn, ABPagina *abPagina) {
    int posEscrever, i;

    posEscrever = (rrn * TAM_PAGINA) + sizeof(int);
    
    fseek(arqAB, posEscrever, SEEK_SET);
    fread(abPagina, TAM_PAGINA, 1, arqAB);

    for(i=0 ; i < (MAX_KEYS + 1) ; i++){
        if(abPagina->filho[i] == NULL){
            abPagina->filho[i] = 0;
        }
    }
}

//CRIAR

int criarPagina(ABPagina *novaPag) {
    int i, j;

    for (i = 0; i < MAX_KEYS; i++) {

        for (j = 0; j < KEY_SIZE - 1; j++)
            novaPag->chave[i][j] = NO_KEY;

        novaPag->chave[i][KEY_SIZE - 1] = '\0';
        novaPag->filho[i] = NIL;
    }

    novaPag->filho[MAX_KEYS] = NIL;

    printf("Pagina criada com sucesso. \n");
}

int criarRaiz(FILE *arqAB, char isbn[KEY_SIZE], int esq, int dir) {
    ABPagina abPagina;
    int rrn;

    rrn = obterPagina(arqAB);
    criarPagina(&abPagina);
    strcpy(abPagina.chave[0], isbn);
    abPagina.filho[0] = esq;
    abPagina.filho[1] = dir;
    abPagina.ctChaves = 1;
    abEscrever(arqAB, rrn, &abPagina);
    inserirHeader(arqAB, rrn);

    printf("Raiz criada com sucesso. \n");

    return rrn;
}

int criarArvoreB(FILE *arqAB, char isbn[KEY_SIZE]) {
    int header = -1;

    fwrite(&header, 1, sizeof(int), arqAB);
    header = criarRaiz(arqAB, isbn, NIL, NIL);

    printf("Arvore criada com sucesso. \n");

    return header;
}

// INSERIR

int inserirHeader(FILE *arqAB, int raiz) {
    rewind(arqAB);
    fwrite(&raiz, 1, sizeof(int), arqAB);

    return raiz;
}

int inserirAB(FILE *arqAB, int rrn, char chave[KEY_SIZE], int *paginaFilhoPromovido, char *chavePromovida) {
    // Função para inserir isbn dentro da árvore

    ABPagina pagina, novaPagina;
    int encontrado, promovido;
    int pos;
    int rrnPromovidoBaixo;
    char chavePromovidaBaixo[KEY_SIZE];

    // se o rrn for igual a -1 (dizendo que ainda nao possui filhos)
    if(rrn == NIL) { 
        strcpy(*chavePromovida, chave);
        *paginaFilhoPromovido = NIL;
        return YES;
    }

    lerArvoreAB(arqAB, rrn, &pagina); // ??????

    // procura se a chave já se encontra na árvore e se não, a posição que deve ser inserido
    encontrado = procurarNo(chave, &pagina, &pos);

    // se a chave já se encontra na árvore retorna um erro
    if(encontrado) {
        printf("ERRO: Chave duplicada (%c)\n", chave);
        return 0;
    }

    printf("Encontrado([0] NAO | [1] SIM)? %d \n", encontrado);

    // ??????? recursividade
    // Acho que essa recursão ta errada, é pra pegar a posição do próx filho
    // chama a função recursivamente até chegar na última camada da árvore
    printf("RRN(%d): %d \nCHAVE: %s\nRRN PROMOVIDO: %d\nCHAVE PROMOVIDO: %s\n==========================\n",pos,  pagina.filho[pos], chave, rrnPromovidoBaixo, chavePromovidaBaixo);
    promovido = inserirAB(arqAB, pagina.filho[pos], chave, &rrnPromovidoBaixo, chavePromovidaBaixo);

    if (!promovido) // não houve promoção
        return NO;

    // Insere sem a necessidade de split se a quantidade de chaves for menor que 4 (ordem 4)
    if(pagina.ctChaves <= MAX_KEYS) {
        insereNaPagina(chave, rrnPromovidoBaixo, &pagina);
        abEscrever(arqAB, rrn, &pagina);

        printf("CHAVE ADICIONADA SEM SPLIT! \n");

        return NO;
    }
    else {
        split(arqAB, chavePromovidaBaixo, rrnPromovidoBaixo, &pagina, (char *) chavePromovida, paginaFilhoPromovido, &novaPagina);
        abEscrever(arqAB, rrn, &pagina);
        abEscrever(arqAB, chavePromovida, &novaPagina);

        printf("ESCRITO COM SPLIT! \n");

        return YES;
    }
}

int insereNaPagina(char chave[KEY_SIZE], int rrnPromovidaBaixo, ABPagina *abPagina) {
    int i;

    /* 
    verificando se a chave é menor que a chave já existente na página e se a posição é diferente de 0
    até achar uma posição para escrever
    */
    for (i = abPagina->ctChaves; strcmp(chave, abPagina->chave[i - 1]) < 0 && i > 0; i--) {
        strcpy(abPagina->chave[i], abPagina->chave[i - 1]);
        abPagina->filho[i + 1] = abPagina->filho[i];
    }

    (abPagina->ctChaves)++;
    strcpy(abPagina->chave[i], chave);
    abPagina->filho[i + 1] = rrnPromovidaBaixo;

    printf("Inserido na Pagina! \n");
}

int procurarNo(char chave[KEY_SIZE], ABPagina *abPagina, int *pos) {
    // retorna a posicao que a chave está se já existir ou a posição que ela deve ser inserida

    int i;

    for (i = 0; i < abPagina->ctChaves && strcmp(chave, abPagina->chave[i]) > 0; i++);

    *pos = i;

    if(*pos < abPagina->ctChaves && strcmp(chave, abPagina->chave[*pos]) == 0)
        return YES; // chave esta na pagina

    return NO; // chave nao esta na pagina
}

// SPLIT

void split(FILE *arqAB, char chave[KEY_SIZE], int rrnPromovidaBaixo, ABPagina *paginaAntiga, char *chavePromovida, int *paginaFilhoPromovido, ABPagina *paginaNova){
    int i;
    int splitPos; // onde sera feito o split
    char splitChaveBuffer[MAX_KEYS + 1][KEY_SIZE];// buffer de chave antes do split
    int splitFilhoBuffer[MAX_KEYS + 2]; // buffer dos filhos antes do split

    for(i = 0; i < MAX_KEYS; i++){
        strcpy(splitChaveBuffer[i], paginaAntiga->chave[i]);
        splitFilhoBuffer[i] = paginaAntiga->filho[i];
    }

    splitFilhoBuffer[i] = paginaAntiga->filho[i];

    for(i = MAX_KEYS; strcmp(chave, splitChaveBuffer[i - 1]) > 0 && i > 0; i--) {
        strcpy(splitChaveBuffer[i], splitChaveBuffer[i-1]);
        splitFilhoBuffer[i+1] = splitFilhoBuffer[i];
    }

    strcpy(splitChaveBuffer[i], chave);
    splitFilhoBuffer[i+1] = rrnPromovidaBaixo;

    *paginaFilhoPromovido = obterPagina(arqAB);

    criarPagina(paginaNova);

    for(i = 0; i < MIN_KEYS; i++){
        //
        strcpy(paginaAntiga->chave[i], splitChaveBuffer[i]);
        paginaAntiga->filho[i] = splitFilhoBuffer[i];

        //
        strcpy(paginaNova->chave[i], splitChaveBuffer[i + 1 + MIN_KEYS]);
        paginaNova->filho[i] = splitFilhoBuffer[i + 1 + MIN_KEYS];

        //
        strcpy(paginaAntiga->chave[i], NO_KEY);
        paginaAntiga->filho[i + 1 + MIN_KEYS] = NIL;
    }

    //
    paginaAntiga->filho[MIN_KEYS] = splitFilhoBuffer[MIN_KEYS];
    paginaNova->filho[MIN_KEYS] = splitFilhoBuffer[i + 1 + MIN_KEYS];

    //
    paginaNova->ctChaves = MAX_KEYS - MIN_KEYS;
    paginaAntiga->ctChaves = MIN_KEYS;

    strcpy(*chavePromovida, splitChaveBuffer[MIN_KEYS]);

    printf("SPLITADO!\n");
}