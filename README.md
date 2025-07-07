# ota_tcp_esp32
Este projeto, **ota_tcp_esp32**, implementa uma solução de atualização Over-The-Air (OTA) via TCP para dispositivos ESP32. O objetivo é permitir a atualização remota do firmware do ESP32 de forma simples e eficiente, utilizando uma conexão TCP-TLS.

## ✅ Funcionalidades

- Atualização OTA do firmware via conexão TCP;
- Comunicação segura e confiável entre o servidor e o ESP32;
- Logs detalhados do processo de atualização;
- Suporte a autenticação básica para maior segurança.

---

## 🗃️ Estrutura do Projeto

- `main/`: Código-fonte principal do firmware ESP32;
- `components/`: Módulos reutilizáveis, como drivers, bibliotecas e middlewares personalizados utilizados pelo firmware;
- `nvs_config/`: Arquivos para configuração da NVS (Non-Volatile Storage) do ESP32;
- `scripts/`: Scripts auxiliares para configuração da NVS;
- `Doxyfile`: Arquivo de configuração para geração automática da documentação com o Doxygen;
- `sdkconfig`: Arquivo de configuração do projeto gerado pelo ESP-IDF;
- `README.md`: Descrição do projeto.

---

## 📥 Requisitos

- ESP32 com suporte a Wi-Fi;
- Espressif IDF instalado para compilação do firmware;
- Instalar dependências do Doxygen:
  ```bash
  sudo apt install doxygen graphviz
  ```
- Criar uma chave PSK de 256 bits para o HMAC;
- Criar uma chave privada e um certificado para a ESP32;
- Definir o SSID e a senha do Wi-Fi no arquivo de configuração wifi_parameters.txt;
- Executar script de configuração e gravação da NVS no ESP32. P/port refere-se a USB da ESP32, s/size o tamanho da partição NVS e o/offset o offset da partição NVS. O script deve ser rodado na raiz do projeto:
  - Linux:
    ```bash
    sh scripts/load_nvs_config.sh -p /dev/ttyUSB0 -s 0x6000 -o 0x9000
    ```
  - Windows:
     ```
    .\scripts\load_nvs_config.ps1 -port COM3 -size 0x6000 -offset 0x9000 
     ```
---

## 🚀 Como Usar

1. Compile e grave o firmware inicial no ESP32;
2. Execute o servidor TCP na máquina local ou remota;
3. Inicie o processo de atualização OTA enviando o novo firmware via TCP;
4. O ESP32 receberá, validará e aplicará a atualização automaticamente;
5. Para gerar a documentação do projeto, executar:
    ```bash
    doxygen Doxyfile
    ```
## Informações Extras:

- Recomenda-se a criação de uma Autoridade Certificadora (CA) local para assinar o certificado da ESP32;
- Para sua criação, gerar um certificado autoassinado por meio da ferramenta Openssl, utilizando uma chave de 4096 bits;
- Criar um CSR para a ESP32 e assinar com a CA local;
- Utilizar o certificado da CA como certificado de confiança nos clientes da ESP32.

---
