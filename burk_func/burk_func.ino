// структура сумматора
typedef struct Sum {
  // выходной сигнал от сумматора
  double u_sum = 0;
} Sum;

// структура коэффициента усиления Gain
typedef struct Gain {
  // выходной сигнал от коэффициента усиления
  double u_gain = 0;
} Gain;

// структура насыщения Saturation
typedef struct Saturation {
  // выходной сигнал от насыщения
  double u_saturation = 0;
} Saturation;

// структура дискретного интегратора
typedef struct Disc_Integ {
  // переменные используемые внутри дискретного интегратора
  double t_before = 0;
  // выходной сигнал от дискретного интегратора
  double u_integ = 0;
} Disc_Integ;

// структура реле
typedef struct Relay {
  // сигнал от реле
  double u_relay = 0;
} Relay;

// структура Ф от тау
typedef struct F_ot_tau {
  // переменные для тау
  // сигнал сохранённый в тау
  double u_tau = 0;
  // Время ВКЛючения двигателя
  unsigned long t_on = 0;
  // Время ВЫКЛючения двигателя
  unsigned long t_off = 0;
  // Логическая переменная (Работает ли двигатель?) - ДА, НЕТ
  // По умолчанию - НЕТ - не работает
  bool is_engine_work = false;
  // Счётчик, сколько раз запускался двигатель
  // По умолчанию 0, так как ни разу еще не запускался
  unsigned long cnt_start = 0;
} F_ot_tau;

// Глобальные переменные
// переменная типа структуры сумматора
Sum sum;
// переменная типа структуры коэффициента усиления
Gain gain;
// переменная типа структуры насыщения
Saturation sat;
// переменная типа структуры дискретного интегратора
Disc_Integ integrator;
// переменная типа структуры реле
Relay relay;
// переменная типа структуры Ф от тау
F_ot_tau ft;
// Флаг что был сигнал stop
bool isStop = false;
// Данные с ПК
String pc_data;

/* Дискретный интегратор */
/* Подаём на вход сигнал */
// in_u - значение сигнала на входе в интегратор
// t - текущее время
void DiscreteIntegratorIn(double in_u, unsigned long t) {

  // Параметры дискретного интегратора
  // k - коэффициент
  double K = 1;
  // Шаг дискретизации
  unsigned long T = 10;

  // Если мы ждали больше шага дискретизации
  if (t - integrator.t_before >= T) {
    // Обновляем значение интеграла
    integrator.u_integ += in_u * K * T * 0.001;  // мс
    // Обновляем сохранённое время
    integrator.t_before = t;
  }
  // Если еще не прождали нужный шаг дискретизации
  // То ничего не делаем, и выводим предыдущий результат

  //u = integrator.u_integ;
}
/* Выходная линия с интеграла */
double DiscreteIntegratorOut() {

  return integrator.u_integ;
}

// Summator
void SummatorIn2(double u1, double u2) {
  // Значение сигнала, полученного Сумматором
  sum.u_sum = u1 + u2;
}

/* Выходная линия с сумматора */
double SummatorOut2() {
  return sum.u_sum;
}
void SummatorIn3(double u1, double u2, double u3) {
  // Значение сигнала, полученного Сумматором
  sum.u_sum = u1 + u2 + u3;
}

/* Выходная линия с сумматора */
double SummatorOut3() {
  return sum.u_sum;
}

// Gain
void GainIn(double in_u) {
  double multiplier = 0.1;
  gain.u_gain = in_u * multiplier;
}

double GainOut() {
  return gain.u_gain;
}

// Saturation
void SaturationIn(double in_u) {
  // Верхняя граница сигнала
  double upper = 0.25;
  // Нижняя граница сигнала
  double lower = -0.25;

  if (in_u >= upper) {
    sat.u_saturation = upper;
  } else if (in_u <= lower) {
    sat.u_saturation = lower;
  } else {
    sat.u_saturation = in_u;
  }
}

/* выходная линия */
double SaturationOut() {
  return sat.u_saturation;
}

// Relay
void RelayIn(double in_u, double sp, double an) {

  // Порог срабатывания реле
  double positive = 0.125;
  double negative = -0.125;

  if (sp < 0.08 && an < 0.4) {
    positive = 0.03;
    negative = -0.03;
  }

  // Преобразуем сигнал либо к -1, либо 0, либо 1
  if (in_u >= positive) {
    relay.u_relay = 1;
  } else if (in_u <= negative) {
    relay.u_relay = -1;
  } else {
    // u принадлежит интервалу (negative, positive )
    relay.u_relay = 0;
  }
}

/* Сигнал который получаем на выходе из Реле */
double RelayOut() {
  return relay.u_relay;
}

// Regulator

void RegulatorIn(unsigned long t, double speed, double angle) {
  SummatorIn2(-angle, -DiscreteIntegratorOut());
  GainIn(SummatorOut2());
  SaturationIn(GainOut());
  SummatorIn3(SaturationOut(), -speed, -RelayOut());
  DiscreteIntegratorIn(SummatorOut3(), t);
  RelayIn(DiscreteIntegratorOut(), speed, angle);
}

/* Сигнал который получаем на выходе из Регулятора */
double RegulatorOut() {
  // Регулятор заканчивается Реле
  return RelayOut();
}

/* Ф от тау */
// F_ot_tau
//  tau - задержку на включение двигателя в [мс]
void F_ot_tauIn(unsigned long t, double in_u) {
  // Параметр тау
  unsigned long tau = 30;

  // Проверяем работает ли двигатель
  // Если работает
  if (ft.is_engine_work) {
    // Находим время работы двигателя
    // Это текущее время минус время запуска двигателя
    unsigned long wake_time = t - ft.t_on;
    // Если  время работы двигателя >30мс
    if (wake_time > tau) {
      // Нужно проверить значение сигнала
      // Если значение сигнала не равно сигналу при включении двигателя
      if (in_u != ft.u_tau) {
        // Нужно выключить двигатель
        ft.is_engine_work = false;
        // Задаём время выключения двигателя - теущее
        ft.t_off = t;
        // сохранить значение сигнала, ставим именно 0
        // Потому что у нас ступенчатый вид сигнала
        // И должна быть его 0 фаза
        // Не может быть чтобы сигнал был,
        // например 0.5 и сразу стал -0.5
        ft.u_tau = 0;
      }
    }
  } else {
    // Двигатель не работает
    // Проверяем запускался ли он хоть раз
    // Если еще не запускался
    if (ft.cnt_start == 0) {
      // То нам не нужно вычислять время спячки двигателя
      // И как только сигнал будет не нулевой
      // Включаем двигатель
      // если текущее значение сигнала НЕ нулевое
      if (in_u != 0) {
        // Включаем двигатель
        ft.is_engine_work = true;
        // Увеличиваем счётчик запусков двигателей
        ft.cnt_start += 1;
        // сохраняем время запуска двигателя - текущее время
        ft.t_on = t;
        // сохранить значение сигнала двигателя при старте
        // - то есть текущее значенеи сигнала
        ft.u_tau = in_u;
      }
    } else {
      // Двигатель УЖЕ запускался
      // Нужно проверить значение сигнала
      // Если сигнал НЕ нулевой
      if (in_u != 0) {
        // Включаем двигатель
        ft.is_engine_work = true;
        // Увеличиваем счётчик запусков двигателей
        ft.cnt_start += 1;
        // сохраняем время запуска двигателя - текущее время
        ft.t_on = t;
        // сохранить значение сигнала двигателя при старте
        // - то есть текущее значение сигнала
        ft.u_tau = in_u;
      }
    }
  }
}

double F_ot_tauOut() {
  return ft.u_tau;
}

// System Система из регулятора и Ф от тау

void SystemIn(String data) {
  // data - это строка, где храниться время, скорость и угол через пробел
  // data = "t speed angle"
  // например: data = "16 3.000 4.000"
  // индекс первого пробела в строке
  int i = data.indexOf(" ");
  // Получаем значение времени
  unsigned long t = data.substring(0, i).toInt();
  // Обрезаем строку, что дальше получить скорость и угол
  data = data.substring(i + 1);
  // Получаем значение скорости
  i = data.indexOf(" ");
  double speed = data.substring(0, i).toDouble();
  // Обрезаем строку, что дальше в данных осталься только угол
  data = data.substring(i + 1);
  // Получаем значение угла
  double angle = data.toDouble();
  SystemRun(t, speed, angle);
}

void SystemRun(unsigned long t, double speed, double angle) {  // Делаем прогон через систему
  // Передаём на вход регулятора значение времени, скорости и угла
  RegulatorIn(t, speed, angle);
  // В Ф(тау) передаём текущее время и выхлоп регулятора
  F_ot_tauIn(t, RegulatorOut());
}
// Что получаем на выходе из системы
double SystemOut() {
  // Система заканчивается Ф от тау
  return F_ot_tauOut();
}
void outPrintln() {
  Serial.println(SystemOut());
}


void setup() {
  SystemIn(pc_data);
  // Настройка вывода
  // инициирует последовательное соединение
  // и задает скорость передачи данных в бит/с (бод)
  Serial.begin(115200);
  // Отправляем сигнал, что готовы к работе
  Serial.println("OK");
}

void loop() {
  // Если не поступало команды stop
  // И можно прочитать данные из серийного порта
  if (!isStop && Serial.available()) {
    // Получаем данные с ПК (формы QT)
    pc_data = Serial.readStringUntil('\n');
    // Если получили команду стоп
    if (pc_data.startsWith("stop")) {
      isStop = true;
    } else {
      // Иначе обычный обмен данных
      // Передаём данные на вход нашей системе
      SystemIn(pc_data);
      // Печатаем в серийный порт ответ от ардуино
      outPrintln();
    }
  }
}