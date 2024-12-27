# EspIrrigação

OK - O usuario recebe a esp32, liga e por default irá iniciar em modo AP (Acontecera verificando o NVS para ver se não tem nenhuma credencial de rede station registrada)

OK - Ele irá se conectar na rede AP da esp32 e enviar as credenciais da rede station pelo servidor TCP com MDNS "esp32-mdns.local"

OK - A esp32 tentará conectar com a rede station, caso sucesso salvar credenciais no NVS, caso erro abrir a rede AP novamente para enviar as credenciais corretas.

Quando conectado a rede station, coletar TIME do servidor NTP (atualizar a cada N minutos/horas)

(analisar) guardar TIME no nvs caso a placa reinicie e não tenha internet (desiscronizado porem so o tempo que ela reinicias)

Conseguir ajustar o TIME e TIMEZONE pelo TCP, conseguindo alterar o horário manualmente, e o TZ para + dinamismo. 

Definir se quer sincronizar ou não o TIME automaticamente, se não atualizar manualmente.

Terá um botão na esp32 que podera resetar as credenciais de rede, apagando o NVS de credencial da rede station e ligando o modo AP

Opção de conetar com bluetooth? Alternar(switch) opção de rede X bluetooth

Deixar servidor TCP sempre aberto para receber e enviar comando para o APP

O APP terá uma mensagem se estiver conectado com o servidor TCP da esp32

APP pode definir frequencia da irrigação que enviará as info para a esp32 que irá guardar no NVS

Se a esp32 estiver na conectado a rede station coletar clima da cidade 

Receber do APP info de irrigação de quais dias e horarios será ligada (ex: seg, qua, sex ; 09h, 20h) guardar no nvs
