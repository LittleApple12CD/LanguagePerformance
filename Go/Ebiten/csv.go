package main

import (
	"encoding/csv"
	"io"
	"strconv"
	"strings"
)

func LoadCSV(data []byte) ([][]float64, error) {
	r := csv.NewReader(strings.NewReader(string(data)))
	r.FieldsPerRecord = -1
	var rows [][]float64
	// 跳过 header
	if _, err := r.Read(); err != nil {
		return nil, err
	}
	for {
		record, err := r.Read()
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, err
		}
		row := make([]float64, len(record))
		for i, s := range record {
			row[i], _ = strconv.ParseFloat(strings.TrimSpace(s), 64)
		}
		rows = append(rows, row)
	}
	return rows, nil
}
