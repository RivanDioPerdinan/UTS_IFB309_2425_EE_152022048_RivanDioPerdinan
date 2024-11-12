from flask import Flask, Response
from flask_cors import CORS
import mysql.connector
import json
from datetime import datetime

app = Flask(__name__)
CORS(app)

db_config = {
    "host": "localhost",
    "user": "root", 
    "password": "",
    "database": "db_iot_bb" 
}

def get_data_from_db():

    conn = mysql.connector.connect(**db_config)
    cursor = conn.cursor(dictionary=True)


    cursor.execute("SELECT MAX(suhu) as suhu_max, MIN(suhu) as suhu_min, AVG(suhu) as suhu_rate FROM tb_cuaca")
    suhu_data = cursor.fetchone()


    cursor.execute("""
        SELECT id as idx, suhu, humid, lux as kecerahan, ts as timestamp
        FROM tb_cuaca
        WHERE suhu = %s AND humid = (SELECT MAX(humid) FROM tb_cuaca WHERE suhu = %s)
        LIMIT 2
    """, (suhu_data['suhu_max'], suhu_data['suhu_max']))
    max_suhu_humid_data = cursor.fetchall()


    for record in max_suhu_humid_data:
        if isinstance(record['timestamp'], datetime):
            record['timestamp'] = record['timestamp'].strftime('%Y-%m-%d %H:%M:%S')


    cursor.execute("""
        SELECT DISTINCT DATE_FORMAT(ts, '%m-%Y') as month_year
        FROM tb_cuaca
        WHERE suhu = %s
        ORDER BY ts ASC
    """, (suhu_data['suhu_max'],))
    month_year_max_data = cursor.fetchall()


    cursor.close()
    conn.close()


    data = {
        "suhu_max": suhu_data['suhu_max'],
        "suhu_min": suhu_data['suhu_min'],
        "suhu_rate": round(suhu_data['suhu_rate'], 2),
        "nilai_suhu_max_humid_max": max_suhu_humid_data,
        "month_year_max": month_year_max_data
    }

    return data

@app.route('/data', methods=['GET'])
def get_data():
    data = get_data_from_db()
    json_data = json.dumps(data, indent=4)
    return Response(json_data, mimetype='application/json')

if __name__ == '__main__':
    app.run(debug=True, port=5000)
