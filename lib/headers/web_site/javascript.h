
#define JAVA_SCRIPT \
"const btEnviar=document.querySelector('#div_inp_minMax button');\n" \
"const E_temp=document.querySelector('#temp');\n" \
"const E_umi=document.querySelector('#umi');\n" \
"const E_pres=document.querySelector('#pr');\n" \
"let dados_R={};\n" \
"btEnviar.addEventListener('click',()=>{\n" \
"let sel=document.getElementById('varSelect').value;\n" \
"let INs=Array.from(document.querySelectorAll('#div_inp_minMax input'));\n" \
"let IN=INs.map(e=>e.value);\n" \
"if(isNaN(IN[0])||isNaN(IN[1])||isNaN(IN[2])){\n" \
"alert('Preencha todos os campos corretamente.');\n" \
"return;\n" \
"}\n" \
"fetch(`/setar_dados/${sel}|${IN[0]}|${IN[1]}|${IN[2]}`);\n" \
"});\n" \
"function Buscar_Dados(){\n" \
"let H3s=Array.from(document.querySelectorAll('#info h3'));\n" \
"let SPANs=H3s.map(e=>e.querySelector('span'));\n" \
"const chaves=['temp','umi','pres'];\n" \
"fetch(`/dados`).then(res=>res.json()).then(data=>{\n" \
"dados_R=data;\n" \
"SPANs.forEach((e,i)=>{\n" \
"e.innerHTML=`<strong>${data[chaves[i]]}</strong>`;\n" \
"});\n" \
"});\n" \
"}\n" \
"function CriarGrafico(){\n" \
"const dados={\n" \
"tempo:[],\n" \
"temp:{dado:[],label:'Temperatura (°C)',bCor:'red'},\n" \
"umi:{dado:[],label:'Umidade (%)',bCor:'blue'},\n" \
"pres:{dado:[],label:'Pressao (hPa)',bCor:'green'}\n" \
"};\n" \
"const ctx=document.getElementById('grafico').getContext('2d');\n" \
"const grafico=new Chart(ctx,{\n" \
"type:'line',\n" \
"data:{\n" \
"labels:dados.tempo,\n" \
"datasets:[{\n" \
"label:dados.temp.label,\n" \
"data:dados.temp.dado,\n" \
"borderColor:dados.temp.bCor,\n" \
"borderWidth:2,\n" \
"fill:false\n" \
"}]\n" \
"},\n" \
"options:{responsive:false}\n" \
"});\n" \
"return [dados,grafico];\n" \
"}\n" \
"function AtualizarGrafico(dados,grafico){\n" \
"const agora=new Date();\n" \
"const horaAtual=agora.toLocaleTimeString();\n" \
"if(dados.tempo.length>=15){\n" \
"dados.tempo.shift();\n" \
"['temp','umi','pres'].forEach(key=>dados[key].dado.shift());\n" \
"}\n" \
"dados.tempo.push(horaAtual);\n" \
"['temp','umi','pres'].forEach(key=>{\n" \
"dados[key].dado.push(Number(dados_R[key])||0);\n" \
"});\n" \
"const varSel=document.getElementById('varSelect').value;\n" \
"grafico.data.datasets[0].data=dados[varSel].dado;\n" \
"grafico.data.datasets[0].label=dados[varSel].label;\n" \
"grafico.data.datasets[0].borderColor=dados[varSel].bCor;\n" \
"grafico.update();\n" \
"}\n" \
"const [dados,grafico]=CriarGrafico();\n" \
"function atualizar_dados(){\n" \
"Buscar_Dados();\n" \
"AtualizarGrafico(dados,grafico);\n" \
"}\n" \
"setInterval(Buscar_Dados,1000);\n" \
"setInterval(()=>AtualizarGrafico(dados,grafico),5000);\n"


