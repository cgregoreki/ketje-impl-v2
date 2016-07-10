# ketje-impl-v2

Este projeto é uma implementação alternativa do algoritmo de criptografia Ketje nas suas versões KetjeJr e KetjeSr. A especificação pode ser encontrada [neste link](http://ketje.noekeon.org/). 

Essa implementação foi construída tendo como objetivo simplificar a leitura do código para que seus passos ficassem mais próximos da especificação. Para conseguir isso, abriu-se mão da versatilidade do algoritmo aceitar *keys* e *nonces* de qualquer tamanho de bits, implementando o caso mais comum que são os *keys* e *nonces* com tamanhos em bits múltiplos de 8. 

## Como ler e entender o código

É interessante que o leitor tenha em mãos a [especificação do algoritmo](http://ketje.noekeon.org/Ketje-1.1.pdf). 

Posteriormente, ao seguir a leitura do código, observar o nome das funções que foram escritas e localizá-las na especificação. Por exemplo, a função *wrap3*, presente tanto em KetjeJr quanto em KetjeSr, está escrita na especificação como parte da construção *MonkeyWrap* com o nome *wrap*. 

Além disso, houve uma tentativa de atingir maior proximidade com a especificação nomeando os argumentos das funções codificadas da mesma maneira que estão escritos no documento. 

Para simplificar a comparação, comentários no código fazem a separação das partes relacionadas na especificação.

## Como executar o código

Este projeto contém um *Makefile* que deve executar a compilação do código.

Portanto, após fazer o download para seu computador, execute:

```
	make
```

Para executar o teste para KetjeJr e KetjeSr, respectivamente, faça:
```
	./bin/ketje/Jr/test_tiny_ketjeJr
	./bin/ketje/Sr/test_tiny_ketjeSr
```
Observe que o *log* de execução foi escrito em um local específico. Para ler os arquivos, para KetjeJr e KetjeSr, respectivamente, faça:

```
	vim bin/ketje/Jr/out_Jr.txt
	vim bin/ketje/Sr/out_Sr.txt
```

O comando `make clean` também está disponível.

## Observações Finais

Este projeto foi alvo de estudo da disciplina *MC030 - Projeto Final de Graduação*, presente ao catálogo do curso de Engenharia de Computação da UNICAMP - Universidade Estadual de Campinas.
