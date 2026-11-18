from machine import Pin, ADC, PWM, I2C
import ssd1306
import time

# ==========================================
# CONFIGURAÇÃO DE HARDWARE (PINOS GPIO)
# ==========================================
# Sensores (Entradas)
sensor_camara = ADC(Pin(34))
sensor_camara.atten(ADC.ATTN_11DB) 

sensor_turbina = ADC(Pin(35))
sensor_turbina.atten(ADC.ATTN_11DB)

# Atuador (Saída)
turbina_pwm = PWM(Pin(15), freq=1000) 

# Visor OLED (Comunicação I2C nos pinos padrão 21 e 22)
i2c = I2C(0, scl=Pin(22), sda=Pin(21))
tela = ssd1306.SSD1306_I2C(128, 64, i2c) # Tela de 128x64 pixels

# ==========================================
# CONSTANTES DO SISTEMA
# ==========================================
SETPOINT = 150.0
RPM_MAX = 3000.0

# ==========================================
# FUNÇÕES DE INTERFACE
# ==========================================
def ler_temperatura(sensor_adc, temp_min, temp_max):
    leitura_bruta = sensor_adc.read()
    temperatura = temp_min + (leitura_bruta / 4095.0) * (temp_max - temp_min)
    return round(temperatura, 1)

def acionar_turbina_pwm(rpm_desejado):
    if rpm_desejado <= 0:
        turbina_pwm.duty(0)
    elif rpm_desejado >= RPM_MAX:
        turbina_pwm.duty(1023)
    else:
        sinal_eletrico = int((rpm_desejado / RPM_MAX) * 1023)
        turbina_pwm.duty(sinal_eletrico)

def atualizar_visor(temp_c, temp_ar, rpm):
    """Limpa a tela e escreve os novos dados do processo."""
    tela.fill(0) # Preenche a tela com 0 (Apaga tudo)
    
    # Escreve os textos. O formato é: texto, eixo X, eixo Y (linha)
    tela.text("MONITORAMENTO", 12, 0)
    tela.text("----------------", 0, 10)
    tela.text(f"Camara: {temp_c} C", 0, 25)
    tela.text(f"Ar    : {temp_ar} C", 0, 40)
    tela.text(f"Turbina: {rpm:.0f} RPM", 0, 55)
    
    tela.show() # Envia as informações da memória para acender os pixels físicos

# ==========================================
# LOOP PRINCIPAL: O CÉREBRO DO CONTROLE
# ==========================================
print("Iniciando Controle com Interface IHM (ESP32)...")
fator_aprendizagem = 1.0 

while True:
    # 1. LER
    temp_camara = ler_temperatura(sensor_camara, 130.0, 160.0)
    temp_ar_turbina = ler_temperatura(sensor_turbina, 20.0, 80.0)
    
    # 2. DECIDIR
    erro = temp_camara - SETPOINT
    
    if erro > 0:
        rpm_calculado = (erro * 500.0) * fator_aprendizagem
        if rpm_calculado > RPM_MAX:
            rpm_calculado = RPM_MAX
    else:
        rpm_calculado = 0.0
        
    # 3. AGIR NO MOTOR
    acionar_turbina_pwm(rpm_calculado)
    
    # 4. EXIBIR (Atualiza a interface visual do operador)
    atualizar_visor(temp_camara, temp_ar_turbina, rpm_calculado)
    
    # Mantemos também o log serial para análise de engenharia
    print(f"Câmara: {temp_camara}°C | Erro: {erro:+.1f}°C | RPM: {rpm_calculado:.0f}")
    
    time.sleep(0.5) # Atualiza o visor a cada meio segundo
