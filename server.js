const express = require('express');
const mongoose = require('mongoose');
const bodyParser = require('body-parser');
const AWS = require('aws-sdk')
const app = express();
const PORT = 3000;
const PDFDocument = require('pdfkit');
const fs=require('fs')
const axios=require('axios')
app.use(bodyParser.json());
const path=require('path')
require('dotenv').config()

const s3 = new AWS.S3({
  accessKeyId: process.env.AWS_ACCESS_KEY,
  secretAccessKey: process.env.AWS_SECRET_KEY
});


mongoose.connect(process.env.MONGODB_URI, {
  useNewUrlParser: true,
  useUnifiedTopology: true,
}).then(() => {
  console.log('Connected to MongoDB');
}).catch(err => {
  console.error('Failed to connect to MongoDB', err);
});

// Define a schema for the payment record
const paymentSchema = new mongoose.Schema({
  amount: { type: Number, required: true },
  timestamp: { type: Date, default: Date.now },
});

// Create a model for the payment record
const Payment = mongoose.model('Payment', paymentSchema);

app.post('/api/payments', async (req, res) => {
  try {
    const { amount } = req.body;
    if (!amount) {
      return res.status(400).json({ message: 'Amount is required' });
    }

    const newPayment = new Payment({ amount });
    await newPayment.save();

    const pdfPath = `./invoices/invoice_${newPayment._id}.pdf`;
    const doc = new PDFDocument();

    const invoiceDir = path.dirname(pdfPath);
    if (!fs.existsSync(invoiceDir)) {
      fs.mkdirSync(invoiceDir, { recursive: true });
    }

    const pdfStream = fs.createWriteStream(pdfPath);
    doc.pipe(pdfStream);

    doc.image('logo.jpg', 50, 45, { width: 100 }).fontSize(20).text('EasyPayVault', 200, 50); 
    doc.moveDown();
    doc.fontSize(12).text(`Invoice for Payment`, { align: 'center' });
    doc.moveDown();
    doc.text(`Amount: INR ${amount}`);
    doc.text(`Timestamp: ${newPayment.timestamp}`);
    doc.moveDown();
    doc.text('Thank you for using EasyPayVault!', { align: 'center' });

    doc.end();

    // Wait until the PDF is completely generated
    pdfStream.on('finish', () => {
      console.log("Hi1");
      const fileContent = fs.readFileSync(pdfPath);

      const params = {
        Bucket: 'upi-iot-project',
        Key: `invoices/invoice_${newPayment._id}.pdf`, // S3 Key (path inside the bucket)
        Body: fileContent,
        ContentType: 'application/pdf'
      };

      // Upload the PDF to S3
      s3.upload(params, (err, data) => {
        console.log("Hi1")
        if (err) {
          console.error('Error uploading file to S3:', err);
          return res.status(500).json({ message: 'Error uploading invoice to S3', error: err });
        }

        // Send back the S3 URL in the response
        res.status(201).json({
          message: 'Payment record created, invoice generated',
          payment_id: newPayment._id,
          s3InvoiceUrl: data.Location // S3 URL of the uploaded PDF
        });
      });
    });

  } catch (error) {
    res.status(500).json({ message: 'Server error', error });
  }
});


app.get('/latest-payment', async (req, res) => {
  try {
    const latestPayment = await Payment.findOne().sort({ timestamp: -1 });
    
    if (latestPayment) {
      res.json({
        amount: latestPayment.amount,
        timestamp: latestPayment.timestamp,
      });
    } else {
      res.status(404).json({ message: 'No payments found' });
    }
  } catch (error) {
    res.status(500).json({ message: 'Server error', error: error.message });
  }
});


app.listen(PORT,'0.0.0.0', () => {
  console.log(`Server running on http://localhost:${PORT}`);
});
