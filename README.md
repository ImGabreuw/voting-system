# Sistema de Votação Distribuído em Tempo Real

**Disciplina:** Computação Distribuída  
**Tecnologias:** C (sockets TCP + threads POSIX)

## Integrantes
- Gabriel Ken Kazama Geronazzo - 10418247
- Lucas Pires de Camargo Sarai - 10418013

---

## Descrição do Projeto

Sistema distribuído de votação em tempo real que permite múltiplos clientes votarem simultaneamente. O servidor gerencia conexões concorrentes, garante voto único por eleitor e fornece placar em tempo real.

### Características Implementadas

✅ Suporte para até 20 clientes simultâneos  
✅ 5 opções de votação configuráveis  
✅ Garantia de voto único por `VOTER_ID`  
✅ Placar parcial em tempo real  
✅ Comando administrativo para encerrar eleição  
✅ Logging completo em `eleicao.log`  
✅ Resultado final exportado para `resultado_final.txt`  
✅ Sincronização com mutexes  
✅ Tolerância a falhas (cliente desconectar após votar)

---

## Arquitetura

### Servidor (`server.c`)
- Aceita conexões TCP na porta 9002
- Gerencia threads para cada cliente
- Sincroniza acesso com mutexes:
  - `clients_mutex`: lista de clientes conectados
  - `voters_mutex`: eleitores que já votaram
  - `votes_mutex`: contadores de votos
  - `log_mutex`: arquivo de log
- Persiste logs e resultado final

### Cliente (`client.c`)
- Interface interativa de linha de comando
- Thread dedicada para receber mensagens do servidor
- Suporta todos os comandos do protocolo

---

## Protocolo de Comunicação

### Cliente → Servidor

| Comando | Descrição |
|---------|-----------|
| `HELLO <VOTER_ID>` | Autenticação inicial |
| `LIST` | Listar opções disponíveis |
| `VOTE <OPCAO>` | Registrar voto |
| `SCORE` | Consultar placar |
| `BYE` | Encerrar conexão |
| `ADMIN CLOSE` | Encerrar eleição (admin) |

### Servidor → Cliente

| Resposta | Descrição |
|----------|-----------|
| `WELCOME <VOTER_ID>` | Confirmação de autenticação |
| `OPTIONS <k> <op1> ... <opk>` | Lista de opções |
| `OK VOTED <OPCAO>` | Voto registrado |
| `ERR DUPLICATE` | Eleitor já votou |
| `ERR INVALID_OPTION` | Opção inválida |
| `ERR CLOSED` | Eleição encerrada |
| `SCORE <k> <op1>:<count1> ...` | Placar parcial |
| `CLOSED FINAL <k> <op1>:<count1> ...` | Placar final |
| `BYE` | Confirmação de desconexão |

---

## Compilação

```bash
# Compilar servidor e cliente
make all

# Compilar apenas o servidor
make server

# Compilar apenas o cliente
make client

# Limpar binários
make clean
```

---

## Execução

### Iniciar o Servidor

```bash
./server
```

O servidor inicia na porta 9002 e aguarda conexões.

### Executar Cliente

```bash
./client
```

Ao conectar, o cliente solicitará o `VOTER_ID` e exibirá o menu de comandos.

### Exemplo de Uso

```
=== Cliente de Votação ===
Conectado ao servidor de votação!

Bem-vindo ao Sistema de Votação!
Envie: HELLO <VOTER_ID> para começar

Digite seu VOTER_ID: eleitor123

=== MENU DE VOTAÇÃO ===
1. LIST         - Listar opções de votação
2. VOTE <opção> - Registrar seu voto
3. SCORE        - Ver placar parcial
4. BYE          - Encerrar sessão
5. ADMIN CLOSE  - Encerrar eleição (admin)

Comando> LIST
OPTIONS 5 Candidato_A Candidato_B Candidato_C Candidato_D Branco

Comando> VOTE Candidato_A
OK VOTED Candidato_A

Comando> SCORE
SCORE 5 Candidato_A:1 Candidato_B:0 Candidato_C:0 Candidato_D:0 Branco:0

Comando> BYE
BYE
```

---

## Testes Realizados

### Casos de Teste Básicos

| Cenário | Comando | Resultado Esperado | Status |
|---------|---------|-------------------|--------|
| Voto único | 2º `VOTE` do mesmo eleitor | `ERR DUPLICATE` | ✅ |
| Opção inválida | `VOTE XYZ` | `ERR INVALID_OPTION` | ✅ |
| 20 clientes simultâneos | 20 conexões | Todos aceitos | ✅ |
| 21º cliente | 21ª conexão | `ERR SERVER_FULL` | ✅ |
| Cliente desconecta após votar | Desconexão | Voto mantido | ✅ |
| Admin encerra eleição | `ADMIN CLOSE` | Novos votos rejeitados | ✅ |
| Consulta pós-encerramento | `SCORE` | `CLOSED FINAL` | ✅ |

### Teste de Concorrência

Execute o script de teste automatizado:

```bash
chmod +x test_voting.sh
./test_voting.sh
```

---

## Estrutura de Arquivos

```
voting-system/
├── server.c              # Código do servidor
├── client.c              # Código do cliente
├── Makefile              # Build configuration
├── README.md             # Este arquivo
├── test_voting.sh        # Script de teste (a criar)
├── eleicao.log           # Log de eventos (gerado)
└── resultado_final.txt   # Resultado final (gerado)
```

---

## Sincronização e Concorrência

### Mutexes Utilizados

1. **`clients_mutex`**: Protege lista de clientes conectados
2. **`voters_mutex`**: Protege lista de eleitores que já votaram
3. **`votes_mutex`**: Protege contadores de votos
4. **`log_mutex`**: Serializa escrita no arquivo de log

### Seções Críticas

- Adição/remoção de clientes
- Verificação de voto duplicado
- Incremento de contadores
- Escrita em arquivo de log

---

## Logs e Resultados

### Arquivo `eleicao.log`

Registra todos os eventos importantes:
- Inicialização do servidor
- Conexões/desconexões de clientes
- Autenticações
- Votos registrados
- Encerramento da eleição

### Arquivo `resultado_final.txt`

Gerado ao executar `ADMIN CLOSE`, contém:
- Data/hora de encerramento
- Total de votos
- Detalhamento por opção
- Percentuais

---

## Observações Técnicas

### Tolerância a Falhas

- Cliente que desconecta após votar: voto é mantido
- Cliente que desconecta antes de votar: nenhum voto registrado
- Servidor mantém estado consistente mesmo com desconexões abruptas

### Limitações Conhecidas

- Porta fixa (9002)
- IP do servidor hardcoded no cliente (127.0.0.1)
- Opções de votação configuradas em código
- Sem autenticação avançada de eleitores

### Possíveis Melhorias

- Configuração via arquivo (porta, opções, lista de eleitores)
- Autenticação com senha
- Interface gráfica (ncurses)
- Exportação para CSV/JSON
- Backup/snapshot do estado
- Distribuição com MPI

---

## Requisitos do Sistema

- GCC com suporte a C99
- POSIX threads (pthread)
- Sistema Unix/Linux
- 64MB RAM (mínimo)

---

## Referências

- POSIX Threads Programming: https://computing.llnl.gov/tutorials/pthreads/
- TCP Socket Programming in C: https://www.geeksforgeeks.org/socket-programming-cc/
- Stevens, W. R. - Unix Network Programming

---

## Licença

Este projeto é desenvolvido para fins acadêmicos na disciplina de Computação Distribuída.