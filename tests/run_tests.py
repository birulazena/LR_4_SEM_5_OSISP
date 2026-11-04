import subprocess
import sys

TEST_RUNS = 5

COMMAND = ["main.exe", "15", "500", "0", "0", "1000", "1"]

print("Запуск автоматических тестов...")
success = True

for i in range(1, TEST_RUNS + 1):
    print(f"Тест {i}/{TEST_RUNS}... ", end="")
    
    try:
        result = subprocess.run(COMMAND, capture_output=True, text=True)
        
        if "Violations: 0" in result.stdout:
            print("УСПЕШНО")
        else:
            print("ОШИБКА!")
            print("Вывод программы:\n", result.stdout)
            success = False
            break
    except FileNotFoundError:
        print("ОШИБКА: Файл main.exe не найден! Убедитесь, что вы скомпилировали программу.")
        success = False
        break

if success:
    print("\nВсе тесты пройдены! Инварианты соблюдаются.")
    sys.exit(0)
else:
    print("\nТесты провалены. Обнаружены состояния гонки или файл не найден.")
    sys.exit(1)