
/*
      this->sum12 = Summator();
      this->gain = Gain(0.1);
      this->sat = Saturation(0.25, -0.25);
      this->sum23 = Summator();
      this->discInteg = DiscreteIntegrator(1, 10);
      this->relay = Relay(0.125, -0.125);
      this->ft = F_ot_tau(30);
*/
/* Дискретный интегратор */
class DiscreteIntegrator
{
  public:
    // сумма входящих сигналов - значение интеграла
    double s = 0;
    // Параметры дискретного интегратора
    double K = 1;
    // Шаг дискретизации
    unsigned long T = 40;
    // Последнее сохранённое время
    unsigned long t_before = 0;

    DiscreteIntegrator()
    {
      this->s = 0;
      // Параметры дискретного интегратора
      this->K = 1;
      // Шаг дискретизации
      this->T = 40;
      // Последнее сохранённое время
      this->t_before = 0;
    }
    /* Конструктор класса */
    // В дискретном интеграторе нужно задавать параметры
    DiscreteIntegrator(double k, unsigned long t)
    {
      this->K = k;
      this->T = t;
    }

    /* Подаём на вход сигнал */
    // in_u - значение сигнала на входе в интегратор
    // t - текущее время
    void in(double in_u, unsigned long t)
    {
      // Если мы ждали больше шага дискретизации
      if (t - this->t_before >= T)
      {
        // Обновляем значение интеграла
        this->s += in_u * this->K * this->T * 0.001;
        // Обновляем сохранённое время
        this->t_before = t;
      }
      // Если еще не прождали нужный шаг дискретизации
      // То ничего не делаем, и выводим предыдущий результат
    }

    /* Выходная линия с интеграла */
    double out()
    {
      return this->s;
    }
};

class Summator
{
  public:
    // Значение сигнала, полученного Сумматором
    double u = 0;

    Summator()
    {
      this->u = 0;
    }

    /* На входе подаём ДВА сигнала */
    void in(double u1, double u2)
    {
      this->u = u1 + u2;
    }

    /* На входе подаём ТРИ сигнала */
    void in(double u1, double u2, double u3)
    {
      this->u = u1 + u2 + u3;
    }

    /* Выходная линия с сумматора */
    double out()
    {
      return this->u;
    }
};

/*Класс Множителя, умножает сигнал на какое-то число*/
class Gain
{
  public:
    // Множитель, по умолчанию = 1, то есть не изменяет сигнал
    double multiplier = 1;
    // Преобразованный сигнал по умолчанию 0
    double u = 0;

    Gain()
    {
      this->u = 0;
      this->multiplier = 1;
    }
    /*Конструктор множителя*/
    Gain(double m)
    {
      // Задаём значение коэффициента умножения
      this->multiplier = m;
    }

    /* На входе подаем значение сигнала */
    void in(double in_u)
    {
      this->u = in_u * this->multiplier;
    }

    /* Выходная линия множителя */
    double out()
    {
      return this->u;
    }
};

/* Класс для насыщения - обрезки значений сигнала */
class Saturation
{
  public:
    // Верхняя граница сигнала
    double upper = 0.1;
    // Нижняя граница сигнала
    double lower = -0.1;
    // Значение сигнала после обрезки
    double u = 0;

    Saturation()
    {
      this->u = 0;
    }
    /* Конструктор насыщения, передаём верхнуюю и нижнюю границы */
    Saturation(double up, double low)
    {
      this->upper = up;
      this->lower = low;
    }

    /* Подаём на входе сигнал */
    void in(double in_u)
    {
      if (in_u >= this->upper)
      {
        this->u = this->upper;
      }
      else if (in_u <= this->lower)
      {
        this->u = this->lower;
      }
      else
      {
        this->u = in_u;
      }
    }

    /* выходная линия */
    double out()
    {
      return this->u;
    }
};

/* Класс представляет собой Реле */
class Relay
{
  public:
    // Значение сигнала, преобразованное Реле
    double u = 0;
    // Порог срабатывания реле
    double positive = 0.1;
    double negative = -0.1;

    Relay()
    {
      this->u = 0;
    }
    /* Конструктор реле, передаём порог срабатывания реле */
    Relay (double pos, double neg)
    {
      this->positive = pos;
      this->negative = neg;
    }
    /* На вход подаём значение сигнала */
    // u - значение сигнала на входе в реле
    void in(double in_u)
    {
      // Преобразуем сигнал либо к -1, либо 0, либо 1
      if (in_u >= this->positive )
      {
        this->u = 1;
      }
      else if (in_u <= this->negative )
      {
        this->u = -1;
      }
      else
      {
        // u принадлежит интервалу (this->negative, this->positive )
        this->u = 0;
      }
    }

    void upd(double line)
    {
      this->positive = line;
      this->negative = - line;
    }

    /* Сигнал который получаем на выходе из Реле */
    double out()
    {
      return this->u;
    }
};

class Regulator
{
  public:
    // Первый сумматор для двух линий (угла и дискретного интегратора)
    Summator sum12;
    // Множитель после сумматора sum12
    Gain gain;
    // Насыщение - обрезатель сигнала
    Saturation sat;
    // Второй сумматор для трёх линий (скорости, насыщение и реле)
    Summator sum23;
    // Дискретный интегратор
    DiscreteIntegrator discInteg;
    // Объект реле, преобразующий значение сигнала
    Relay relay;
    Regulator()
    {
      this->sum12 = Summator();
      this->gain = Gain(0.1);
      this->sat = Saturation(0.25, -0.25);
      this->sum23 = Summator();
      this->discInteg = DiscreteIntegrator(1, 10);
      this->relay = Relay(0.125, -0.125);
    }
    /* Что подаём на входе в Регулятор */
    // t - текущее время от запуска программы
    // speed - значение скорости из объекта управления
    // angle - значение угла из объекта управления
    void in(unsigned long t, double speed, double angle)
    {
      // Делаем проход по регулятору
      if (speed < 0.08 && angle < 0.4)
      {
        this->relay.upd(0.03);
      }
      this->sum12.in(-angle, -this->discInteg.out());
      this->gain.in(this->sum12.out());
      this->sat.in(this->gain.out());
      this->sum23.in(this->sat.out(), -speed, -this->relay.out());
      this->discInteg.in(this->sum23.out(), t);
      this->relay.in(this->discInteg.out());
    }
    /* Сигнал который получаем на выходе из Регулятора */
    double out()
    {
      // Регулятор заканчивается Реле
      return this->relay.out();
    }
};

/* Класс объекта Ф от тау */
class F_ot_tau
{
  public:
    // Параметр регулятора
    unsigned long tau = 30;
    // Время ВКЛючения двигателя
    unsigned long t_on = 0;
    // Время ВЫКЛючения двигателя
    unsigned long t_off = 0;
    // Логическая переменная (Работает ли двигатель?) - ДА, НЕТ
    // По умолчанию - НЕТ - не работает
    bool is_engine_work = false;
    //Счётчик, сколько раз запускался двигатель
    // По умолчанию 0, так как ни разу еще не запускался
    unsigned long cnt_start = 0;
    // Значение изменённого сигнала в F_ot_tau
    double u = 0;

    /* Конструктор пустого класса */
    F_ot_tau()
    {
      // Параметр регулятора
      this->tau = 30;
      // Время ВКЛючения двигателя
      this->t_on = 0;
      // Время ВЫКЛючения двигателя
      this->t_off = 0;
      // Логическая переменная (Работает ли двигатель?) - ДА, НЕТ
      // По умолчанию - НЕТ - не работает
      this->is_engine_work = false;
      //Счётчик, сколько раз запускался двигатель
      // По умолчанию 0, так как ни разу еще не запускался
      this->cnt_start = 0;
      // Значение изменённого сигнала в F_ot_tau
      this->u = 0;
    }
    /* Конструктор класса */
    // Принимает параметры
    // tau - задержку на включение двигателя в [мс]
    F_ot_tau(unsigned long tau)    
    {
      // Параметр тау
      this->tau = tau;
      // Время ВКЛючения двигателя
      this->t_on = 0;
      // Время ВЫКЛючения двигателя
      this->t_off = 0;
      // Логическая переменная (Работает ли двигатель?) - ДА, НЕТ
      // По умолчанию - НЕТ - не работает
      this->is_engine_work = false;
      //Счётчик, сколько раз запускался двигатель
      // По умолчанию 0, так как ни разу еще не запускался
      this->cnt_start = 0;
      // Значение изменённого сигнала в F_ot_tau
      this->u = 0;
    }

    /* На вход подаём сигнал и время */
    void in(unsigned long t, double in_u)
    {
      // Проверяем работает ли двигатель
      // Если работает
      if (this->is_engine_work)
      {
        // Находим время работы двигателя
        // Это текущее время минус время запуска двигателя
        unsigned long wake_time = t - t_on;
        // Если  время работы двигателя >30мс
        if (wake_time > this->tau)
        {
          // Нужно проверить значение сигнала
          // Если значение сигнала не равно сигналу при включении двигателя
          if (in_u != this->u)
          {
            // Нужно выключить двигатель
            this->is_engine_work = false;
            // Задаём время выключения двигателя - теущее
            this->t_off = t;
            // сохранить значение сигнала, ставим именно 0
            // Потому что у нас ступенчатый вид сигнала
            // И должна быть его 0 фаза
            // Не может быть чтобы сигнал был,
            // например 0.5 и сразу стал -0.5
            this->u = 0;
          }
        }
      }
      else
      {
        // Двигатель не работает
        // Проверяем запускался ли он хоть раз
        // Если еще не запускался
        if (this->cnt_start == 0)
        {
          // То нам не нужно вычислять время спячки двигателя
          // И как только сигнал будет не нулевой
          // Включаем двигатель
          // если текущее значение сигнала НЕ нулевое
          if (in_u != 0)
          {
            // Включаем двигатель
            this->is_engine_work = true;
            // Увеличиваем счётчик запусков двигателей
            this->cnt_start += 1;
            // сохраняем время запуска двигателя - текущее время
            this->t_on = t;
            // сохранить значение сигнала двигателя при старте
            // - то есть текущее значенеи сигнала
            this->u = in_u;
          }
        }
        else
        {
            // Двигатель УЖЕ запускался
            // Нужно проверить значение сигнала
            // Если сигнал НЕ нулевой
            if (in_u != 0)
            {
              // Включаем двигатель
              this->is_engine_work = true;
              // Увеличиваем счётчик запусков двигателей
              this->cnt_start += 1;
              // сохраняем время запуска двигателя - текущее время
              this->t_on = t;
              // сохранить значение сигнала двигателя при старте
              // - то есть текущее значение сигнала
              this->u = in_u;
            }
        }
      }
    }

    double out()
    {
      return this->u;
    }
};

/*Класс системы на Arduino*/
class ASystem
{
  public:
    // Сам регулятор
    Regulator regulator;
    // Управляющее воздействие
    F_ot_tau ft;

    ASystem()
    {
      this->regulator = Regulator();
      this->ft = F_ot_tau(30);
    }

    // на вход передаеём строку из серийного порта
    void in(String data)
    {
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

      // Делаем прогон через систему
      this->run(t, speed, angle);
    }

    // Что получаем на выходе из системы
    double out()
    {
      // Система заканчивается Ф от тау
      return this->ft.out();
    }

    // Что выводим в серийный порт
    void outPrintln()
    {
      Serial.println(this->out());
    }

  private:
    // Внутренняя функция. НЕ для внешнего воздействия
    // Отличается от in тем, что передаются
    // вещественные переменные, а не строка как в методе in
    // Передаём время, скорость, угол
    void run(unsigned long t, double speed, double angle)
    {
      // Делаем прогон через систему
      // Передаём на вход регулятора значение времени, скорости и угла
      this->regulator.in(t, speed, angle);
      // В Ф(тау) передаём текущее время и выхлоп регулятора
      this->ft.in(t, this->regulator.out());
    }
};

/* Глобальные переменые */
// Переменная системы, крутящейся на ардуино
ASystem my_as;
// Данные с ПК
String pc_data;
// Флаг что был сигнал stop
bool isStop;

void setup()
{
  // Создаём новую систему
  my_as = ASystem();
  // сигнала stop не было
  isStop = false;
  // Настройка вывода
  // инициирует последовательное соединение
  // и задает скорость передачи данных в бит/с (бод)
  Serial.begin(115200);
  // Отправляем сигнал, что готовы к работе
  Serial.println("OK");
}

void loop()
{
  // Если не поступало комманды stop
  // И можно прочитать данные из серийного порта
  if (!isStop && Serial.available())
  {
    // Получаем данные с ПК (формы QT)
      pc_data = Serial.readStringUntil('\n');
    // Если получили команду стоп
    if (pc_data.startsWith("stop"))
    {
      isStop = true;
    }
    else
    {
      // Иначе обычный обмен данных
      // Передаём данные на вход нашей системе
      my_as.in(pc_data);
      // Печатаем в серийный порт ответ от ардуино
      my_as.outPrintln();
    }
  }
}
