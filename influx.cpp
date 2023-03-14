#include "influx.h"
#include "credentials.h"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB);
/* Use a mutex to prevent both tasks from writing at the same time */
/* which causes a lock up */
SemaphoreHandle_t mutex = xSemaphoreCreateMutex();

void setupInfluxOptions() {
  client.setHTTPOptions(HTTPOptions().connectionReuse(true));
}

bool validateInfluxConnection() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  bool influxOk = client.validateConnection();
  xSemaphoreGive(mutex);
  if (influxOk) {
    Serial.print(F("[ INFLUX ] Connected to: "));
    Serial.println(client.getServerUrl());
    return true;
  } else {
    Serial.print(F("[ INFLUX ] Connection failed: "));
    Serial.println(client.getLastErrorMessage());
    return false;
  }
}
