const btEnviar = document.querySelector('#div_inp_minMax button');
const E_temp = document.querySelector('#temp');
const E_umi = document.querySelector('#umi');
const E_pres = document.querySelector('#pr');

let dados_R = {};
let uri = "http://192.168.1.110";

btEnviar.addEventListener('click', () => {
    let sel = document.getElementById('varSelect').value; 
    let INs = Array.from(document.querySelectorAll('#div_inp_minMax input'));
    let IN = INs.map(e => e.value);

    // Validação
    if (isNaN(IN[0]) || isNaN(IN[1]) || isNaN(IN[2])) {
        alert('Preencha todos os campos corretamente.');
        return;
    }

    // Exemplo de uso: exibir os dados
    console.log(`Variável: ${sel}`);
    console.log(`Mínimo: ${IN[0]}`);
    console.log(`Máximo: ${IN[1]}`);
    console.log(`Offset: ${IN[2]}`);

    fetch(`${uri}/setar_dados/${sel}|${IN[0]}|${IN[1]}|${IN[2]}`); 
});

function Buscar_Dados() {
    let H3s = Array.from(document.querySelectorAll('#info h3'));
    let SPANs = H3s.map(e => e.querySelector("span"));
    const chaves = ["temp", "umi", "pres"];
    fetch(`${uri}/dados`)
        .then(res => res.json())
        .then(data => {
            dados_R = data;
            SPANs.forEach((e,i) => {
                e.innerHTML = `<strong>${data[chaves[i]]}</strong>`;
            });
        });
}

function CriarGrafico(){
    const dados = {
        tempo: [],
        temp: { dado: [], label: "Temperatura (°C)", bCor: "red" },
        umi: { dado: [], label: "Umidade (%)", bCor: "blue" },
        pres: { dado: [], label: "Pressao (hPa)", bCor: "green" }
    };

    const ctx = document.getElementById('grafico').getContext('2d');
    const grafico = new Chart(ctx, {
        type: 'line',
        data: {
            labels: dados.tempo,
            datasets: [{
                label: dados.temp.label,
                data: dados.temp.dado,
                borderColor: dados.temp.bCor,
                borderWidth: 2,
                fill: false
            }]
        },
        options: { responsive: false }
    });

    return [dados, grafico];
}

// Função para atualizar gráfico
function AtualizarGrafico(dados, grafico){
    const agora = new Date();
    const horaAtual = agora.toLocaleTimeString();

    if (dados.tempo.length >= 15) {
        dados.tempo.shift();
        ["temp", "umi", "pres"].forEach(key => dados[key].dado.shift());
    }
    dados.tempo.push(horaAtual);
    ["temp", "umi", "pres"].forEach(key => {
        dados[key].dado.push(Number(dados_R[key]) || 0); 
    });

    const varSel = document.getElementById('varSelect').value;
    grafico.data.datasets[0].data = dados[varSel].dado;
    grafico.data.datasets[0].label = dados[varSel].label;
    grafico.data.datasets[0].borderColor = dados[varSel].bCor;
    grafico.update();
}

const [dados, grafico] = CriarGrafico();

function atualizar_dados(){
    Buscar_Dados();
    AtualizarGrafico(dados, grafico);
}

setInterval(Buscar_Dados, 1000); // Atualiza os dados
setInterval(() => AtualizarGrafico(dados, grafico), 5000); // Atualiza o gráfico
