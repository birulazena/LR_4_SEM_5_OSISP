import csv
import matplotlib.pyplot as plt
import os

data = []
try:
    with open('data/results.csv', 'r') as file:
        reader = csv.reader(file)
        for row in reader:
            data.append([int(x) for x in row])
except FileNotFoundError:
    print("Файл data/results.csv не найден. Сначала запустите C++ программу.")
    exit()

configs = {}
for row in data:
    key = f"{row[0]} фил., {row[2]} мс"
    if key not in configs:
        configs[key] = {'total_time': [], 'wait_time': []}
    configs[key]['total_time'].append(row[4])
    configs[key]['wait_time'].append(row[5])

labels = list(configs.keys())
avg_total = [sum(configs[k]['total_time'])/len(configs[k]['total_time']) for k in labels]
avg_wait = [sum(configs[k]['wait_time'])/len(configs[k]['wait_time']) for k in labels]

plt.figure(figsize=(10, 5))
plt.bar(labels, avg_total, color='skyblue')
plt.title('Зависимость времени выполнения от параметров модели')
plt.ylabel('Время выполнения (мс)')
plt.xlabel('Конфигурация (Кол-во философов, Время размышления)')
plt.grid(axis='y', linestyle='--')
plt.savefig('diagrams/total_time.png')

plt.figure(figsize=(10, 5))
plt.bar(labels, avg_wait, color='salmon')
plt.title('Зависимость времени ожидания ресурса от параметров')
plt.ylabel('Среднее время ожидания (мс)')
plt.xlabel('Конфигурация (Кол-во философов, Время размышления)')
plt.grid(axis='y', linestyle='--')
plt.savefig('diagrams/wait_time.png')

print("Графики успешно сохранены в папку diagrams/")