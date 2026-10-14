#include <Arduino.h>

/*
==========================================================
       SEMÁFORO INTELIGENTE - ESP32 DEVKIT V1
==========================================================

VIA A:
GPIO 26 = Vermelho
GPIO 27 = Amarelo
GPIO 14 = Verde
GPIO 13 = Botão/Sensor A

VIA B:
GPIO 32 = Vermelho
GPIO 33 = Amarelo
GPIO 25 = Verde
GPIO 34 = Botão/Sensor B

Cada pressionamento do botão = 1 veículo

CONTROLE:
- Conta veículos somente nos primeiros 15 segundos
  de cada período de farol verde.
- Compara o fluxo atual com o fluxo dos primeiros
  15 segundos da outra via no verde anterior.
- O próximo tempo de verde será 30, 35 ou 40 segundos.
==========================================================
*/


// ==========================================================
//                    DEFINIÇÃO DOS PINOS
// ==========================================================

// ---------------- VIA A ----------------

const byte VERMELHO_A = 26;
const byte AMARELO_A  = 27;
const byte VERDE_A    = 14;

const byte BOTAO_A    = 13;


// ---------------- VIA B ----------------

const byte VERMELHO_B = 32;
const byte AMARELO_B  = 33;
const byte VERDE_B    = 25;

const byte BOTAO_B    = 34;


// ==========================================================
//                    VARIÁVEIS DE CONTAGEM
// ==========================================================

// Quantidade de veículos nos primeiros 15 segundos

volatile unsigned int fluxoA15 = 0;
volatile unsigned int fluxoB15 = 0;


// Fluxo dos primeiros 15 segundos do verde anterior

unsigned int fluxoAAnterior = 0;
unsigned int fluxoBAnterior = 0;


// Indica qual via está contando

volatile bool contandoA = false;
volatile bool contandoB = false;


// ==========================================================
//                       DEBOUNCE
// ==========================================================

volatile unsigned long ultimoPulsoA = 0;
volatile unsigned long ultimoPulsoB = 0;

const unsigned long DEBOUNCE = 200;


// ==========================================================
//                    INTERRUPÇÃO BOTÃO A
// ==========================================================

void IRAM_ATTR contarVeiculoA()
{
  unsigned long agora = millis();

  if (agora - ultimoPulsoA >= DEBOUNCE)
  {
    ultimoPulsoA = agora;

    if (contandoA)
    {
      fluxoA15++;
    }
  }
}


// ==========================================================
//                    INTERRUPÇÃO BOTÃO B
// ==========================================================

void IRAM_ATTR contarVeiculoB()
{
  unsigned long agora = millis();

  if (agora - ultimoPulsoB >= DEBOUNCE)
  {
    ultimoPulsoB = agora;

    if (contandoB)
    {
      fluxoB15++;
    }
  }
}


// ==========================================================
//                 CALCULAR TEMPO DE VERDE
// ==========================================================

int calcularTempoVerde(unsigned int fluxoAtual,
                       unsigned int fluxoAnterior)
{

  // --------------------------------------------------------
  // OUTRA VIA NÃO TEVE NENHUM VEÍCULO
  // --------------------------------------------------------

  if (fluxoAnterior == 0)
  {
    if (fluxoAtual == 0)
    {
      return 30;
    }

    return 40;
  }


  // --------------------------------------------------------
  // FLUXO ATUAL <= 1,3 x FLUXO ANTERIOR
  // --------------------------------------------------------

  if (fluxoAtual <= 1.3 * fluxoAnterior)
  {
    return 30;
  }


  // --------------------------------------------------------
  // 1,3 x ANTERIOR < ATUAL <= 1,6 x ANTERIOR
  // --------------------------------------------------------

  else if (fluxoAtual <= 1.6 * fluxoAnterior)
  {
    return 35;
  }


  // --------------------------------------------------------
  // FLUXO ATUAL > 1,6 x FLUXO ANTERIOR
  // --------------------------------------------------------

  else
  {
    return 40;
  }
}


// ==========================================================
//                         SETUP
// ==========================================================

void setup()
{

  // ---------------- VIA A ----------------

  pinMode(VERMELHO_A, OUTPUT);
  pinMode(AMARELO_A, OUTPUT);
  pinMode(VERDE_A, OUTPUT);


  // ---------------- VIA B ----------------

  pinMode(VERMELHO_B, OUTPUT);
  pinMode(AMARELO_B, OUTPUT);
  pinMode(VERDE_B, OUTPUT);


  // ---------------- BOTÕES ----------------

  // Mantém INPUT porque o circuito original
  // utiliza resistores externos.

  pinMode(BOTAO_A, INPUT);
  pinMode(BOTAO_B, INPUT);


  // ---------------- INTERRUPÇÕES ----------------

  attachInterrupt(
    digitalPinToInterrupt(BOTAO_A),
    contarVeiculoA,
    RISING
  );

  attachInterrupt(
    digitalPinToInterrupt(BOTAO_B),
    contarVeiculoB,
    RISING
  );


  // ---------------- MONITOR SERIAL ----------------

  Serial.begin(115200);


  // ========================================================
  //                    ESTADO INICIAL
  // ========================================================

  // VIA A = VERMELHO

  digitalWrite(VERMELHO_A, HIGH);
  digitalWrite(AMARELO_A, LOW);
  digitalWrite(VERDE_A, LOW);


  // VIA B = VERMELHO

  digitalWrite(VERMELHO_B, HIGH);
  digitalWrite(AMARELO_B, LOW);
  digitalWrite(VERDE_B, LOW);


  delay(1000);
}


// ==========================================================
//                         LOOP
// ==========================================================

void loop()
{

  // ========================================================
  //                    VIA A - VERDE
  // ========================================================

  noInterrupts();

  fluxoA15 = 0;

  interrupts();


  // Ativa somente a contagem da VIA A
  contandoA = true;
  contandoB = false;


  // --------------------------------------------------------
  // VIA A = VERDE
  // VIA B = VERMELHO
  // --------------------------------------------------------
  digitalWrite(VERMELHO_A, LOW);
  digitalWrite(AMARELO_A, LOW);
  digitalWrite(VERDE_A, HIGH);

  digitalWrite(VERMELHO_B, HIGH);
  digitalWrite(AMARELO_B, LOW);
  digitalWrite(VERDE_B, LOW);


  Serial.println();
  Serial.println("================================");
  Serial.println("          VIA A - VERDE");
  Serial.println("================================");
  Serial.println("Contando veiculos por 15 segundos...");


  // ========================================================
  //                PRIMEIROS 15 SEGUNDOS
  // ========================================================

  unsigned long inicioA = millis();

  while (millis() - inicioA < 15000)
  {
    // A interrupção do botão A realiza a contagem.
  }


  // ========================================================
  //                  FINAL DOS 15 SEGUNDOS
  // ========================================================

  contandoA = false;


  noInterrupts();

  unsigned int carrosA15 = fluxoA15;

  interrupts();


  // ========================================================
  //             CALCULAR TEMPO DE VERDE A
  // ========================================================

  int tempoVerdeA =
    calcularTempoVerde(carrosA15, fluxoBAnterior);


  Serial.print("Veiculos A nos primeiros 15s: ");
  Serial.println(carrosA15);

  Serial.print("Veiculos B nos primeiros 15s do verde anterior: ");
  Serial.println(fluxoBAnterior);

  Serial.print("Tempo de verde da VIA A: ");
  Serial.print(tempoVerdeA);
  Serial.println(" segundos");


  // ========================================================
  //                TEMPO RESTANTE DO VERDE A
  // ========================================================

  int restanteA = tempoVerdeA - 15;

  if (restanteA > 0)
  {
    delay((unsigned long)restanteA * 1000);
  }


  // ========================================================
  //                     TERMINA VIA A
  // ========================================================

  fluxoAAnterior = carrosA15;


  // ========================================================
  //                     AMARELO A
  // ========================================================

  digitalWrite(VERDE_A, LOW);
  digitalWrite(AMARELO_A, HIGH);

  delay(3000);

  digitalWrite(AMARELO_A, LOW);
  digitalWrite(VERMELHO_A, HIGH);


  // ========================================================
  //                    VIA B - VERDE
  // ========================================================

  noInterrupts();

  fluxoB15 = 0;

  interrupts();


  contandoB = true;
  contandoA = false;


  // --------------------------------------------------------
  // VIA B = VERDE
  // VIA A = VERMELHO
  // --------------------------------------------------------

  digitalWrite(VERMELHO_B, LOW);
  digitalWrite(AMARELO_B, LOW);
  digitalWrite(VERDE_B, HIGH);

  digitalWrite(VERMELHO_A, HIGH);
  digitalWrite(AMARELO_A, LOW);
  digitalWrite(VERDE_A, LOW);


  Serial.println();
  Serial.println("================================");
  Serial.println("          VIA B - VERDE");
  Serial.println("================================");
  Serial.println("Contando veiculos por 15 segundos...");


  // ========================================================
  //                PRIMEIROS 15 SEGUNDOS
  // ========================================================

  unsigned long inicioB = millis();

  while (millis() - inicioB < 15000)
  {
    // A interrupção do botão B realiza a contagem.
  }


  // ========================================================
  //                  FINAL DOS 15 SEGUNDOS
  // ========================================================

  contandoB = false;


  noInterrupts();

  unsigned int carrosB15 = fluxoB15;

  interrupts();


  // ========================================================
  //             CALCULAR TEMPO DE VERDE B
  // ========================================================

  int tempoVerdeB =
    calcularTempoVerde(carrosB15, fluxoAAnterior);


  Serial.print("Veiculos B nos primeiros 15s: ");
  Serial.println(carrosB15);

  Serial.print("Veiculos A nos primeiros 15s do verde anterior: ");
  Serial.println(fluxoAAnterior);

  Serial.print("Tempo de verde da VIA B: ");
  Serial.print(tempoVerdeB);
  Serial.println(" segundos");


  // ========================================================
  //                TEMPO RESTANTE DO VERDE B
  // ========================================================

  int restanteB = tempoVerdeB - 15;

  if (restanteB > 0)
  {
    delay((unsigned long)restanteB * 1000);
  }


  // ========================================================
  //                     TERMINA VIA B
  // ========================================================

  fluxoBAnterior = carrosB15;


  // ========================================================
  //                     AMARELO B
  // ========================================================

  digitalWrite(VERDE_B, LOW);
  digitalWrite(AMARELO_B, HIGH);

  delay(3000);

  digitalWrite(AMARELO_B, LOW);
  digitalWrite(VERMELHO_B, HIGH);


  // Pequeno intervalo antes de reiniciar
  delay(500);
}