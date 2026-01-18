import sqlite3
conn = sqlite3.connect('D:/my_telegram_bot/db/bot_data.db')
c = conn.cursor()
c.execute("SELECT ID, USER_ID, TARIFF, NAME, PRICE, PHONE, EMAIL, ADDRESS, TIMESTAMP, STATUS FROM applications ORDER BY ID DESC")
rows = c.fetchall()
print(f"Found {len(rows)} applications")
for row in rows:
    print(f"ID={row[0]}, Name={row[3]}, Status={row[9]}")
