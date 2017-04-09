// primos.cpp : Defines the entry point for the console application.
//

#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<process.h>
#include<time.h>
#include <vector>
#include<iostream>
#include<math.h>
#include<string>
#include<sstream>
#include<cstdio>

using namespace std;

typedef struct {
	int passageiroId;
	bool ocupado;
	int indice;
}t_Assento;

typedef struct {
	vector<t_Assento> vetAssentos;
	int qtdAssentos;
}t_Assentos;

typedef struct {
	int operacao;
	int passageiroId;
	int indiceAssento;
}Operacao;//struct parametro do arquivo Log

typedef struct {
	int id;
	bool sentado;
}Passageiro;

void inicializar();
void threadRegistrarLog(void* threadId);
void passageiro(void* threadId);
int filaLog(vector<int>parametros, t_Assentos assentosLocal);
void escreverLog(int operacao, Passageiro passageiro, t_Assento assento);
void visualizarAssentos(Passageiro passageiro);
int alocarAssentoLivre(t_Assento* ptrAssento, Passageiro* passageiroId);
int alocarAssentoDado(t_Assento* assentoDado, Passageiro* passageiroId);
int liberarAssentoDado(t_Assento* assento, Passageiro* passageiroId);
string verificarArgumentos(int argc, char *argv[], string &nomeArquivo);
void finalizar(/*HANDLE* hThread, Passageiro* passageiros*/);

HANDLE operacaoMutex;//seção critica paraescrever Log

HANDLE escrever;

HANDLE pronto;

HANDLE threadLog;

t_Assentos assentos;

vector<Operacao> filaOperacoes;

bool flag = false;

string nomeArquivo;

int main(int argc, char *argv[])
{
	string erro;

	erro = verificarArgumentos(argc, argv, nomeArquivo);
	if (!erro.empty())
	{
		fprintf(stderr, erro.c_str());
		system("pause");
		exit(-1);
	}
	HANDLE* hThread;
	hThread = new HANDLE[assentos.qtdAssentos];
	int idThreadLog;
	Passageiro* passageiros;
	passageiros = new Passageiro[assentos.qtdAssentos];

	inicializar();

	printf("Starting Threads!\n");

	idThreadLog = 0;
	threadLog = (HANDLE)_beginthread(threadRegistrarLog, 0, &idThreadLog);

	for (int i=0; i < assentos.qtdAssentos; i++)//criando passageiros
	{
		passageiros[i].id = i+1;
		passageiros[i].sentado = false;
		hThread[i] = (HANDLE)_beginthread(passageiro, 0, &passageiros[i]);
	}

	printf("\nProcessando...\n\n");

	WaitForMultipleObjects(assentos.qtdAssentos, hThread, true, INFINITE);
	flag = true;
	SetEvent(escrever);
	WaitForSingleObject(threadLog, INFINITE);

	finalizar(/*hThread, passageiros*/);

	system("pause");
	return 0;
}

string verificarArgumentos(int argc, char *argv[], string &nomeArquivo)
{
	size_t flagExtensao;
	if (argc != 3)
	{
		return "A entrada deve ter dois parametros, o nome do arquivo e a quantidade de assentos!\n";
	}
	nomeArquivo = argv[1];
	flagExtensao = nomeArquivo.find(".txt");
	if (flagExtensao == string::npos)
		nomeArquivo += ".txt";
	assentos.qtdAssentos = atoi(argv[2]);
	return "";
}

void inicializar() {
	char opcao;

	escrever = CreateEvent(NULL, TRUE, FALSE, NULL);
	pronto = CreateEvent(NULL, TRUE, FALSE, NULL);
	operacaoMutex = CreateMutex(NULL, false, NULL);

	for (int i = 0; i < assentos.qtdAssentos; i++)
	{
		t_Assento assento;
		assento.ocupado = false;
		assento.passageiroId = 0;
		assento.indice = i+1;
		assentos.vetAssentos.push_back(assento);
	}
	remove(nomeArquivo.c_str());//Exclui arquivo se houver!
}

void finalizar(/*HANDLE* hThread, Passageiro* passageiros*/) 
{
	CloseHandle(escrever);
	CloseHandle(pronto);
	CloseHandle(operacaoMutex);
	TerminateThread(threadLog, 0);
	//free(hThread);
	//free(passageiro);
}

void threadRegistrarLog(void* threadId)
{
	vector<int> parametros;

	while (true) 
	{
		t_Assentos assentosLocal;
		if (filaOperacoes.size() == 0)
			WaitForSingleObject(escrever, INFINITE);//dorme
		ResetEvent(escrever);
		if (filaOperacoes.size() == 0 && flag)
			_endthread();

		Operacao operacaoLog;
		operacaoLog = filaOperacoes[0];
		filaOperacoes.erase(filaOperacoes.begin());
		assentosLocal = assentos;
		SetEvent(pronto);

		//preparando parametros
		parametros.push_back(operacaoLog.operacao);
		parametros.push_back(operacaoLog.passageiroId);
		if (operacaoLog.indiceAssento != -1)
			parametros.push_back(operacaoLog.indiceAssento);

		filaLog(parametros, assentosLocal);

		parametros.clear();
		if (filaOperacoes.size() == 0 && flag)
			_endthread();
		
	}

}

void passageiro(void* ptrPassageiro) 
{
	int operacoes;//Cada passageiro ira realizar de 1 a 10 operacoes
	int operacao;//operacao aleatoria
	Passageiro passageiro = *((Passageiro*)ptrPassageiro);
	t_Assento* assento = NULL;

	srand((unsigned)time(NULL)+(unsigned)passageiro.id);
	operacoes = (rand() % 10) + 1;
	for (int i = 0; i < operacoes; i++)
	{
		operacao = rand() % 4;
		if (WaitForSingleObject(operacaoMutex, INFINITE) != WAIT_FAILED)
		{
			switch (operacao)
			{
			case 0://vizualizar assento
				visualizarAssentos(passageiro);
				break;
			case 1://alocar assento livre
				alocarAssentoLivre(assento, &passageiro);
				break;
			case 2://alocar assento dado
				assento = &assentos.vetAssentos[rand() % assentos.qtdAssentos];
				alocarAssentoDado(assento, &passageiro);
				break;
			case 3://liberar assento dado
				assento = &assentos.vetAssentos[rand() % assentos.qtdAssentos];
				liberarAssentoDado(assento, &passageiro);
			}
		}
		ReleaseMutex(operacaoMutex);
	}
	_endthread();
}

int filaLog(vector<int>parametros, t_Assentos assentosLocal)
{
	FILE *arquivo;
	int i, j;
	string fraseLog;
	ostringstream stream;

	arquivo = fopen(nomeArquivo.c_str(), "a+t");

	if (arquivo == NULL)
	{
		perror("Erro em abrir o arquivo de Log!\n");
		return -1;
	}

	for (int i = 0; i < parametros.size(); i++)
	{
		stream << parametros[i] << ",";
	}

	stream << "[" << assentosLocal.vetAssentos[0].passageiroId;
	for (int i = 1; i < assentosLocal.vetAssentos.size(); i++)
	{
		stream << "," << assentosLocal.vetAssentos[i].passageiroId;
	}
	stream << "]";
	fraseLog = stream.str();
	fraseLog += "\0";

	fprintf(arquivo, "%s\n", &fraseLog[0]);
	fclose(arquivo);

	return 0;
}

void escreverLog(int operacao, Passageiro passageiro, t_Assento assento)
{
	int indiceAssento = assento.indice;

	if (indiceAssento > 0)//alterando atributos assentos
	{
		assentos.vetAssentos[indiceAssento - 1] = assento;
	}
	//preparando parametros
	Operacao operacaoLog;
	operacaoLog.operacao = operacao;
	operacaoLog.passageiroId = passageiro.id;
	operacaoLog.indiceAssento = indiceAssento;
	filaOperacoes.push_back(operacaoLog);

	SetEvent(escrever);//acordando threadLog
	WaitForSingleObject(pronto, INFINITE);
	ResetEvent(pronto);
}

void visualizarAssentos(Passageiro passageiro) {
	int operacao = 1;
	t_Assento assento;
	assento.indice = -1;

	escreverLog(operacao, passageiro, assento);
}

int alocarAssentoLivre(t_Assento* ptrAssento,Passageiro* passageiro)
{
	vector<int> indicesAssentosLivres;
	int aux, indice;
	int operacao = 2;
	t_Assento assento;
	assento.indice = 0;

	for (int i = 0; i < assentos.vetAssentos.size(); i++)
		if (assentos.vetAssentos[i].ocupado == false)
			indicesAssentosLivres.push_back(i);

	if (indicesAssentosLivres.size() != 0)//existe assento livre
	{
		aux = rand() % indicesAssentosLivres.size();//selecionando um indice do vetor de indices;
		indice = indicesAssentosLivres[aux];
		assento = assentos.vetAssentos[indice];
	}
	if (indicesAssentosLivres.size() == 0||passageiro->sentado)//não há assentos livres
	{
		escreverLog(operacao, *passageiro, assento);
		return 0;
	}

	//alocando assento
	assento.ocupado = true;
	assento.passageiroId = passageiro->id;
	ptrAssento = &assentos.vetAssentos[assento.indice-1];
	passageiro->sentado = true;
	escreverLog(operacao, *passageiro, assento);
	return 1;
}

int alocarAssentoDado(t_Assento* assentoDado, Passageiro* passageiro) 
{
	int operacao = 3;
	if (assentoDado->ocupado||passageiro->sentado)
	{
		escreverLog(operacao, *passageiro, *assentoDado);
		return 0;
	}
	assentoDado->passageiroId = passageiro->id;
	assentoDado->ocupado = true;
	passageiro->sentado = true;
	escreverLog(operacao, *passageiro, *assentoDado);
	return 1;
}

int liberarAssentoDado(t_Assento* assento, Passageiro* passageiro)
{
	int operacao = 4;

	if (assento->passageiroId != passageiro->id || !passageiro->sentado)
	{
		escreverLog(operacao, *passageiro, *assento);
		return 0;
	}
	assento->ocupado = false;
	assento->passageiroId = 0;
	passageiro->sentado = false;
	escreverLog(operacao, *passageiro, *assento);
	return 1;
}