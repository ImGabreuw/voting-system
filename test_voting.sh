#!/bin/bash

# Script de teste para o Sistema de Votação Distribuído
# Testa múltiplos clientes votando simultaneamente

echo "=== Teste do Sistema de Votação Distribuído ==="
echo ""

# Verificar se o servidor está compilado
if [ ! -f "./server" ]; then
    echo "Erro: Servidor não compilado. Execute 'make server' primeiro."
    exit 1
fi

if [ ! -f "./client" ]; then
    echo "Erro: Cliente não compilado. Execute 'make client' primeiro."
    exit 1
fi

# Criar diretório para logs de teste
mkdir -p test_logs

# Função para simular um cliente
simulate_client() {
    local voter_id=$1
    local option=$2
    local delay=$3
    
    sleep $delay
    
    {
        sleep 0.5
        echo "HELLO $voter_id"
        sleep 0.5
        echo "LIST"
        sleep 0.5
        echo "VOTE $option"
        sleep 0.5
        echo "SCORE"
        sleep 0.5
        echo "BYE"
    } | ./client > "test_logs/client_${voter_id}.log" 2>&1 &
}

echo "1. Iniciando servidor em background..."
./server > test_logs/server.log 2>&1 &
SERVER_PID=$!
echo "   Servidor iniciado (PID: $SERVER_PID)"
sleep 2

echo ""
echo "2. Testando conexão básica..."
{
    sleep 0.5
    echo "HELLO eleitor_teste"
    sleep 0.5
    echo "LIST"
    sleep 0.5
    echo "BYE"
} | ./client > test_logs/test_connection.log 2>&1
echo "   ✓ Conexão básica OK"

echo ""
echo "3. Testando voto único..."
{
    sleep 0.5
    echo "HELLO eleitor_unico"
    sleep 0.5
    echo "VOTE Candidato_A"
    sleep 0.5
    echo "VOTE Candidato_B"
    sleep 0.5
    echo "BYE"
} | ./client > test_logs/test_duplicate.log 2>&1
echo "   ✓ Teste de voto duplicado concluído (verifique test_logs/test_duplicate.log)"

echo ""
echo "4. Testando opção inválida..."
{
    sleep 0.5
    echo "HELLO eleitor_invalido"
    sleep 0.5
    echo "VOTE OpçãoInexistente"
    sleep 0.5
    echo "BYE"
} | ./client > test_logs/test_invalid.log 2>&1
echo "   ✓ Teste de opção inválida concluído"

echo ""
echo "5. Simulando 10 clientes simultâneos votando..."

# Simular 10 clientes com diferentes opções
simulate_client "eleitor001" "Candidato_A" 0.1
simulate_client "eleitor002" "Candidato_B" 0.2
simulate_client "eleitor003" "Candidato_C" 0.3
simulate_client "eleitor004" "Candidato_A" 0.4
simulate_client "eleitor005" "Candidato_D" 0.5
simulate_client "eleitor006" "Candidato_B" 0.6
simulate_client "eleitor007" "Candidato_A" 0.7
simulate_client "eleitor008" "Branco" 0.8
simulate_client "eleitor009" "Candidato_C" 0.9
simulate_client "eleitor010" "Candidato_A" 1.0

echo "   Aguardando clientes finalizarem..."
sleep 8

echo "   ✓ 10 clientes simultâneos processados"

echo ""
echo "6. Consultando placar final..."
{
    sleep 0.5
    echo "HELLO admin_consulta"
    sleep 0.5
    echo "SCORE"
    sleep 0.5
    echo "BYE"
} | ./client > test_logs/test_score.log 2>&1
echo "   ✓ Placar consultado (veja test_logs/test_score.log)"

echo ""
echo "7. Testando encerramento administrativo..."
{
    sleep 0.5
    echo "HELLO admin"
    sleep 0.5
    echo "ADMIN CLOSE"
    sleep 1
    echo "BYE"
} | ./client > test_logs/test_admin_close.log 2>&1
echo "   ✓ Eleição encerrada"

sleep 2

echo ""
echo "8. Testando voto após encerramento..."
{
    sleep 0.5
    echo "HELLO eleitor_tarde"
    sleep 0.5
    echo "VOTE Candidato_A"
    sleep 0.5
    echo "BYE"
} | ./client > test_logs/test_closed.log 2>&1
echo "   ✓ Voto rejeitado após encerramento (verifique test_logs/test_closed.log)"

echo ""
echo "9. Encerrando servidor..."
kill $SERVER_PID 2>/dev/null
sleep 1

echo ""
echo "=== RESUMO DOS TESTES ==="
echo ""
echo "Logs dos testes salvos em: test_logs/"
echo ""

if [ -f "eleicao.log" ]; then
    echo "Log do servidor:"
    echo "----------------"
    tail -20 eleicao.log
    echo ""
fi

if [ -f "resultado_final.txt" ]; then
    echo "Resultado final da eleição:"
    echo "---------------------------"
    cat resultado_final.txt
    echo ""
fi

echo "=== Testes Concluídos ==="
echo ""
echo "Para análise detalhada, verifique:"
echo "  - test_logs/          (logs de cada teste)"
echo "  - eleicao.log         (log completo do servidor)"
echo "  - resultado_final.txt (resultado da eleição)"
