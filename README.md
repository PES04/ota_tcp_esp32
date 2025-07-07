# ota_tcp_esp32
Este projeto, **ota_tcp_esp32**, implementa uma solução de atualização Over-The-Air (OTA) via TCP para dispositivos ESP32. O objetivo é permitir a atualização remota do firmware do ESP32 de forma simples e eficiente, utilizando uma conexão TCP-TLS.

## ✅ Funcionalidades

- Atualização OTA do firmware via conexão TCP.
- Comunicação segura e confiável entre o servidor e o ESP32.
- Logs detalhados do processo de atualização.
- Suporte a autenticação básica para maior segurança.

---

## 🗃️ Estrutura do Projeto

- `main/`: Código-fonte principal do firmware ESP32.
- `components/`: Módulos reutilizáveis, como drivers, bibliotecas e middlewares personalizados utilizados pelo firmware.
- `nvs_config/`: Arquivos para configuração da NVS (Non-Volatile Storage) do ESP32.
- `scripts/`: Scripts auxiliares para configuração da NVS.
- `Doxyfile`: Arquivo de configuração para geração automática da documentação com o Doxygen.
- `sdkconfig`: Arquivo de configuração do projeto gerado pelo ESP-IDF.
- `docs/`: Documentação adicional do projeto.
- `README.md`: Descrição do projeto.

---

## 📥 Requisitos

- ESP32 com suporte a Wi-Fi.
- Espressif IDF instalado para compilação do firmware.
- Instalar dependências do Doxygen:
  ```bash
  sudo apt install doxygen graphviz
  ```
- Executar script de configuração da NVS (GNU/Linux) no ESP32:
  ```bash
  sh scripts/load_nvs_config.sh -p /dev/ttyUSB0 -s 0x6000 -o 0x9000
  ```

---

## 🚀 Como Usar

1. Compile e grave o firmware inicial no ESP32.
2. Execute o servidor TCP na máquina local ou remota.
3. Inicie o processo de atualização OTA enviando o novo firmware via TCP.
4. O ESP32 receberá, validará e aplicará a atualização automaticamente.
5. Para gerar a documentação do projeto, executar:
    ```bash
    doxygen Doxyfile
    ```

---
